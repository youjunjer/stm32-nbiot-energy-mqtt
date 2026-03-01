#include <Arduino.h>
#include <SoftwareSerial.h>

#define DBG_PORT Serial
// SoftSerial for energy module: RX=D4, TX=D5
SoftwareSerial energyUart(D4, D5);

static const uint8_t kReadCmd[] = {0xF8, 0x04, 0x00, 0x00, 0x00, 0x0A, 0x64, 0x64};
static const uint32_t kPollIntervalMs = 60000;
static uint32_t g_lastPollMs = 0;
static uint32_t g_lastBlinkMs = 0;
static uint32_t g_failCount = 0;

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
  if (HAL_RCC_OscConfig(&osc) != HAL_OK) Error_Handler();

  clk.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                  RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  clk.AHBCLKDivider = RCC_SYSCLK_DIV1;
  clk.APB1CLKDivider = RCC_HCLK_DIV1;
  clk.APB2CLKDivider = RCC_HCLK_DIV1;
  if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_1) != HAL_OK) Error_Handler();
}

static void printHex(const uint8_t *buf, size_t n)
{
  DBG_PORT.print("raw=");
  for (size_t i = 0; i < n; i++) {
    if (buf[i] < 0x10) DBG_PORT.print('0');
    DBG_PORT.print(buf[i], HEX);
    DBG_PORT.print(' ');
  }
  DBG_PORT.println();
}

static bool isEchoFrame(const uint8_t *buf, size_t n)
{
  if (n != sizeof(kReadCmd)) return false;
  for (size_t i = 0; i < sizeof(kReadCmd); i++) {
    if (buf[i] != kReadCmd[i]) return false;
  }
  return true;
}

static size_t readOnce(uint8_t *buf, size_t cap)
{
  energyUart.listen();
  while (energyUart.available() > 0) energyUart.read();

  DBG_PORT.print("tx=");
  for (size_t i = 0; i < sizeof(kReadCmd); i++) {
    if (kReadCmd[i] < 0x10) DBG_PORT.print('0');
    DBG_PORT.print(kReadCmd[i], HEX);
    DBG_PORT.print(' ');
  }
  DBG_PORT.println();

  energyUart.write(kReadCmd, sizeof(kReadCmd));
  delay(100); // same style as ESP32 code

  size_t n = 0;
  while (energyUart.available() > 0 && n < cap) {
    int c = energyUart.read();
    if (c >= 0) buf[n++] = (uint8_t)c;
  }
  return n;
}

static size_t readEnergyFrame(uint8_t *buf, size_t cap)
{
  size_t n = readOnce(buf, cap);

  // If first read is pure echo, retry once immediately.
  if (isEchoFrame(buf, n)) {
    DBG_PORT.println("echo detected, retry once...");
    delay(20);
    n = readOnce(buf, cap);
  }
  return n;
}

static bool decodeAll(const uint8_t *b, size_t n, float &v, float &a, float &w, float &kwh)
{
  if (n < 17) return false;

  uint16_t vRaw = ((uint16_t)b[3] << 8) | b[4];
  uint32_t aRaw = ((uint32_t)b[7] << 24) | ((uint32_t)b[8] << 16) | ((uint32_t)b[5] << 8) | b[6];
  uint32_t wRaw = ((uint32_t)b[11] << 24) | ((uint32_t)b[12] << 16) | ((uint32_t)b[9] << 8) | b[10];
  uint32_t eRaw = ((uint32_t)b[15] << 24) | ((uint32_t)b[16] << 16) | ((uint32_t)b[13] << 8) | b[14];

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

  DBG_PORT.begin(115200);
  energyUart.begin(9600);
  energyUart.listen();
  delay(300);

  DBG_PORT.println();
  DBG_PORT.println("=== 09_energy_softserial_read ===");
  DBG_PORT.println("Energy SoftSerial: RX=D4, TX=D5, 9600 8N1");
  DBG_PORT.println("Poll every 10 seconds");
}

void loop()
{
  uint32_t now = millis();
  if (now - g_lastBlinkMs >= 500) {
    g_lastBlinkMs = now;
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  }

  if (g_lastPollMs != 0 && (now - g_lastPollMs) < kPollIntervalMs) {
    return;
  }
  g_lastPollMs = now;

  uint8_t frame[64];
  size_t n = readEnergyFrame(frame, sizeof(frame));

  DBG_PORT.print("rx_bytes=");
  DBG_PORT.println((int)n);

  if (n == 0) {
    g_failCount++;
    DBG_PORT.println("no response");
    if (g_failCount >= 3) {
      DBG_PORT.println("SOFTSERIAL_LINK_FAIL");
    }
    return;
  }

  printHex(frame, n);

  float v = 0, a = 0, w = 0, e = 0;
  if (decodeAll(frame, n, v, a, w, e)) {
    g_failCount = 0;
    DBG_PORT.print("V="); DBG_PORT.print(v, 1);
    DBG_PORT.print(" V, I="); DBG_PORT.print(a, 3);
    DBG_PORT.print(" A, P="); DBG_PORT.print(w, 1);
    DBG_PORT.print(" W, E="); DBG_PORT.print(e, 3);
    DBG_PORT.println(" kWh");
  } else {
    g_failCount++;
    DBG_PORT.println("decode fail");
    if (g_failCount >= 3) {
      DBG_PORT.println("SOFTSERIAL_LINK_FAIL");
    }
  }
}
