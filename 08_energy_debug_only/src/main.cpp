#include <Arduino.h>
#include <SoftwareSerial.h>

#define DBG_PORT Serial
// Energy module via SoftwareSerial to free D2/D8.
// RX pin = D4, TX pin = D5
SoftwareSerial energyUart(D4, D5);

static const uint8_t kReadCmd[] = {0xF8, 0x04, 0x00, 0x00, 0x00, 0x0A, 0x64, 0x64};
static const uint32_t kPollIntervalMs = 10000;
static uint32_t g_lastPollMs = 0;
static uint32_t g_lastBlinkMs = 0;

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

  clk.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  clk.AHBCLKDivider = RCC_SYSCLK_DIV1;
  clk.APB1CLKDivider = RCC_HCLK_DIV1;
  clk.APB2CLKDivider = RCC_HCLK_DIV1;
  if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_1) != HAL_OK) Error_Handler();
}

static size_t readFrame(uint8_t *buf, size_t cap, uint32_t timeoutMs)
{
  while (energyUart.available() > 0) energyUart.read();
  energyUart.write(kReadCmd, sizeof(kReadCmd));

  uint32_t start = millis();
  size_t n = 0;
  while ((millis() - start) < timeoutMs && n < cap) {
    while (energyUart.available() > 0 && n < cap) {
      int c = energyUart.read();
      if (c >= 0) buf[n++] = (uint8_t)c;
    }
    delay(2);
  }
  return n;
}

static bool decode(const uint8_t *b, size_t n, float &v, float &a, float &w, float &e)
{
  if (n < 17) return false;

  uint16_t vRaw = ((uint16_t)b[3] << 8) | b[4];
  uint32_t aRaw = ((uint32_t)b[7] << 24) | ((uint32_t)b[8] << 16) | ((uint32_t)b[5] << 8) | b[6];
  uint32_t wRaw = ((uint32_t)b[11] << 24) | ((uint32_t)b[12] << 16) | ((uint32_t)b[9] << 8) | b[10];
  uint32_t eRaw = ((uint32_t)b[15] << 24) | ((uint32_t)b[16] << 16) | ((uint32_t)b[13] << 8) | b[14];

  v = vRaw * 0.1f;
  a = aRaw * 0.001f;
  w = wRaw * 0.1f;
  e = eRaw * 0.001f;
  return true;
}

static void printRaw(const uint8_t *buf, size_t n)
{
  DBG_PORT.print("raw=");
  for (size_t i = 0; i < n; i++) {
    if (buf[i] < 0x10) DBG_PORT.print('0');
    DBG_PORT.print(buf[i], HEX);
    DBG_PORT.print(' ');
  }
  DBG_PORT.println();
}

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  DBG_PORT.begin(115200);
  energyUart.begin(9600);
  delay(300);

  DBG_PORT.println();
  DBG_PORT.println("=== 08_energy_debug_only ===");
  DBG_PORT.println("Energy UART: SoftwareSerial RX=D4, TX=D5, 9600 8N1");
  DBG_PORT.println("Debug UART: COM13 115200 8N1");
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
  size_t n = readFrame(frame, sizeof(frame), 250);

  if (n == 0) {
    DBG_PORT.println("no response");
    return;
  }

  printRaw(frame, n);

  float v = 0, a = 0, w = 0, e = 0;
  if (decode(frame, n, v, a, w, e)) {
    DBG_PORT.print("V=");
    DBG_PORT.print(v, 1);
    DBG_PORT.print(" V, I=");
    DBG_PORT.print(a, 3);
    DBG_PORT.print(" A, P=");
    DBG_PORT.print(w, 1);
    DBG_PORT.print(" W, E=");
    DBG_PORT.print(e, 3);
    DBG_PORT.println(" kWh");
  } else {
    DBG_PORT.println("decode fail (frame too short)");
  }
}
