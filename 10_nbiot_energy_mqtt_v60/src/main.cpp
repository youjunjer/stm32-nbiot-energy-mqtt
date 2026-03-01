#include <Arduino.h>
#include <string.h>
#include <SoftwareSerial.h>

#define PC_PORT Serial
HardwareSerial nbiot(PA10, PA9); // RX=D2, TX=D8
static const uint8_t kNbiotPwrPin = PA8; // D7, LOW=power off, HIGH=power on
// Energy module on SoftwareSerial to avoid UART conflict with NB-IoT
SoftwareSerial energyUart(D4, D5); // RX=D4, TX=D5

static const char *kDomain = "mqttgo.io";
static const char *kIp = "218.161.43.149";
static const char *kPort = "1883";
static const char *kTimeout = "60000";
static const char *kBuffer = "1024";
static const char *kClientId = "6545451212";
static const char *kTopic = "csu/energy/data";
static const uint8_t kEnergyReadCmd[] = {0xF8, 0x04, 0x00, 0x00, 0x00, 0x0A, 0x64, 0x64};

static char rxBuf[1200];
static bool g_ready = false;
static uint32_t g_lastPubMs = 0;
static uint32_t g_lastEnergyTryMs = 0;
static const uint32_t kEnergyRetryMs = 2000;

extern "C" void SystemClock_Config(void)
{
  RCC_OscInitTypeDef osc = {};
  RCC_ClkInitTypeDef clk = {};

  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  osc.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  osc.HSIState = RCC_HSI_ON;
  osc.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  osc.PLL.PLLState = RCC_PLL_ON;
  osc.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  osc.PLL.PLLMUL = RCC_PLLMUL_6;
  osc.PLL.PLLDIV = RCC_PLLDIV_3;
  if (HAL_RCC_OscConfig(&osc) != HAL_OK) {
    Error_Handler();
  }

  clk.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                  RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  clk.AHBCLKDivider = RCC_SYSCLK_DIV1;
  clk.APB1CLKDivider = RCC_HCLK_DIV1;
  clk.APB2CLKDivider = RCC_HCLK_DIV1;
  if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_1) != HAL_OK) {
    Error_Handler();
  }
}

static void clearRx()
{
  while (nbiot.available() > 0) {
    nbiot.read();
  }
}

static bool waitResponse(uint32_t timeoutMs)
{
  memset(rxBuf, 0, sizeof(rxBuf));
  size_t idx = 0;
  uint32_t start = millis();
  bool got = false;

  while ((millis() - start) < timeoutMs) {
    while (nbiot.available() > 0) {
      int c = nbiot.read();
      if (c < 0) {
        continue;
      }
      got = true;
      if (idx < sizeof(rxBuf) - 1U) {
        rxBuf[idx++] = (char)c;
      }
    }
    if (strstr(rxBuf, "\r\nOK\r\n") != nullptr || strstr(rxBuf, "\r\nERROR\r\n") != nullptr) {
      break;
    }
    delay(2);
  }

  PC_PORT.println("[RX]");
  if (got) {
    PC_PORT.println(rxBuf);
  } else {
    PC_PORT.println("(no response)");
  }

  return strstr(rxBuf, "\r\nOK\r\n") != nullptr;
}

static bool sendCmd(const char *cmd, uint32_t timeoutMs)
{
  clearRx();
  PC_PORT.print("[TX] ");
  PC_PORT.println(cmd);
  nbiot.print(cmd);
  nbiot.print("\r\n");
  return waitResponse(timeoutMs);
}

static bool checkCereg()
{
  bool ok = sendCmd("AT+CEREG?", 3000);
  if (!ok) {
    return false;
  }

  const char *p = strstr(rxBuf, "+CEREG:");
  if (!p) {
    return false;
  }

  // Accept any <n>,1 or <n>,5
  return strstr(p, ",1") != nullptr || strstr(p, ",5") != nullptr;
}

static void powerCycleNbiot()
{
  PC_PORT.println("[PWR] NB-IoT power OFF (LOW) for 5s");
  digitalWrite(kNbiotPwrPin, LOW);
  delay(5000);

  PC_PORT.println("[PWR] NB-IoT power ON (HIGH)");
  digitalWrite(kNbiotPwrPin, HIGH);

  // Give modem time to boot and output UART banner.
  delay(5000);
}

static void ensureAtiReady()
{
  while (true) {
    bool ok = sendCmd("ATI", 3000);
    PC_PORT.println(ok ? "[STEP ATI] OK" : "[STEP ATI] FAIL");
    if (ok) {
      return;
    }
    powerCycleNbiot();
  }
}

static bool waitNetworkRegistered(uint32_t maxWaitMs)
{
  uint32_t start = millis();
  while ((millis() - start) < maxWaitMs) {
    bool ok = checkCereg();
    PC_PORT.println(ok ? "[STEP CEREG] REGISTERED" : "[STEP CEREG] NOT REGISTERED");
    if (ok) {
      return true;
    }
    delay(5000);
  }
  return false;
}

static bool mqttConnectSequence()
{
  char cmd[256];

  snprintf(cmd, sizeof(cmd), "AT+EDNS=\"%s\"", kDomain);
  if (!sendCmd(cmd, 5000)) {
    PC_PORT.println("[STEP EDNS] FAIL");
    return false;
  }
  PC_PORT.println("[STEP EDNS] OK");

  snprintf(cmd, sizeof(cmd), "AT+EMQNEW=%s,%s,%s,%s", kIp, kPort, kTimeout, kBuffer);
  if (!sendCmd(cmd, 5000)) {
    PC_PORT.println("[STEP EMQNEW] FAIL");
    return false;
  }
  PC_PORT.println("[STEP EMQNEW] OK");

  // Must follow EMQNEW quickly (<10s per modem requirement)
  snprintf(cmd, sizeof(cmd), "AT+EMQCON=0,3.1,\"%s\",60000,0,0,\"\",\"\"", kClientId);
  if (!sendCmd(cmd, 8000)) {
    PC_PORT.println("[STEP EMQCON] FAIL");
    return false;
  }
  PC_PORT.println("[STEP EMQCON] OK");
  return true;
}

static void restartFromAti(const char *reason)
{
  PC_PORT.println();
  PC_PORT.print("[RECOVER] ");
  PC_PORT.println(reason);
  g_ready = false;
  g_lastPubMs = 0;
  g_lastEnergyTryMs = 0;

  while (true) {
    ensureAtiReady();

    if (!waitNetworkRegistered(300000)) {
      PC_PORT.println("[STEP CEREG] TIMEOUT 300s, reboot modem");
      powerCycleNbiot();
      continue;
    }

    if (!mqttConnectSequence()) {
      PC_PORT.println("[RECOVER] MQTT setup failed, reboot modem");
      powerCycleNbiot();
      continue;
    }

    PC_PORT.println("=== CONNECTED ===");
    PC_PORT.println("Auto publish energy JSON every 60 seconds.");
    g_ready = true;
    return;
  }
}

static void bytesToHex(const char *src, size_t len, char *hexOut, size_t hexOutSize)
{
  static const char table[] = "0123456789abcdef";
  size_t out = 0;
  for (size_t i = 0; i < len; i++) {
    if ((out + 2) >= hexOutSize) {
      break;
    }
    uint8_t b = (uint8_t)src[i];
    hexOut[out++] = table[(b >> 4) & 0x0F];
    hexOut[out++] = table[b & 0x0F];
  }
  hexOut[out] = '\0';
}

static bool isEchoFrame(const uint8_t *buf, size_t n)
{
  if (n != sizeof(kEnergyReadCmd)) return false;
  for (size_t i = 0; i < sizeof(kEnergyReadCmd); i++) {
    if (buf[i] != kEnergyReadCmd[i]) return false;
  }
  return true;
}

static size_t readEnergyOnce(uint8_t *buf, size_t cap)
{
  energyUart.listen();
  while (energyUart.available() > 0) energyUart.read();

  energyUart.write(kEnergyReadCmd, sizeof(kEnergyReadCmd));
  delay(100); // same behavior as working ESP32 version

  size_t n = 0;
  while (energyUart.available() > 0 && n < cap) {
    int c = energyUart.read();
    if (c >= 0) buf[n++] = (uint8_t)c;
  }
  return n;
}

static bool readEnergy(float &v, float &a, float &w, float &kwh)
{
  uint8_t frame[64];
  size_t n = readEnergyOnce(frame, sizeof(frame));
  if (isEchoFrame(frame, n)) {
    delay(20);
    n = readEnergyOnce(frame, sizeof(frame));
  }
  if (n < 17) {
    return false;
  }

  uint16_t vRaw = ((uint16_t)frame[3] << 8) | frame[4];
  uint32_t aRaw = ((uint32_t)frame[7] << 24) | ((uint32_t)frame[8] << 16) | ((uint32_t)frame[5] << 8) | frame[6];
  uint32_t wRaw = ((uint32_t)frame[11] << 24) | ((uint32_t)frame[12] << 16) | ((uint32_t)frame[9] << 8) | frame[10];
  uint32_t eRaw = ((uint32_t)frame[15] << 24) | ((uint32_t)frame[16] << 16) | ((uint32_t)frame[13] << 8) | frame[14];

  v = vRaw * 0.1f;
  a = aRaw * 0.001f;
  w = wRaw * 0.1f;
  kwh = eRaw * 0.001f;
  return true;
}

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  pinMode(kNbiotPwrPin, OUTPUT);
  digitalWrite(kNbiotPwrPin, HIGH);

  PC_PORT.begin(115200);
  nbiot.begin(115200);
  energyUart.begin(9600);
  energyUart.listen();
  delay(500);

  PC_PORT.println();
  PC_PORT.println("=== 06_nbiot_mqtt_hello ===");
  restartFromAti("BOOT");
}

void loop()
{
  if (!g_ready) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(80);
    digitalWrite(LED_BUILTIN, LOW);
    delay(920);
    return;
  }

  uint32_t now = millis();
  bool duePublish = (g_lastPubMs == 0) || ((now - g_lastPubMs) >= 60000);
  if (duePublish) {
    if (g_lastEnergyTryMs != 0 && (now - g_lastEnergyTryMs) < kEnergyRetryMs) {
      goto blink_part;
    }
    g_lastEnergyTryMs = now;

    float v = 0, a = 0, w = 0, kwh = 0;
    if (!readEnergy(v, a, w, kwh)) {
      PC_PORT.println("[ENERGY] read fail, retry...");
      goto blink_part;
    }

    char aStr[20], vStr[20], wStr[20], kwhStr[20];
    dtostrf(a, 0, 3, aStr);
    dtostrf(v, 0, 1, vStr);
    dtostrf(w, 0, 1, wStr);
    dtostrf(kwh, 0, 3, kwhStr);

    char jsonPayload[160];
    snprintf(jsonPayload, sizeof(jsonPayload), "{\"A\":%s,\"V\":%s,\"W\":%s,\"kWh\":%s}", aStr, vStr, wStr, kwhStr);
    PC_PORT.print("[ENERGY] ");
    PC_PORT.println(jsonPayload);

    char hexPayload[512];
    bytesToHex(jsonPayload, strlen(jsonPayload), hexPayload, sizeof(hexPayload));
    int hexLen = (int)strlen(hexPayload);

    char cmd[768];
    snprintf(cmd, sizeof(cmd), "AT+EMQPUB=0,%s,0,0,0,%d,%s", kTopic, hexLen, hexPayload);
    bool ok = sendCmd(cmd, 5000);
    PC_PORT.println(ok ? "[STEP EMQPUB] OK" : "[STEP EMQPUB] FAIL");
    if (!ok) {
      restartFromAti("EMQPUB failed, restart full flow");
    } else {
      g_lastPubMs = now;
      g_lastEnergyTryMs = 0;
    }
  }

  // heartbeat blink while connected
blink_part:
  static uint32_t lastBlink = 0;
  if (now - lastBlink >= 500) {
    lastBlink = now;
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  }
}
