#include <Arduino.h>

#define PC_PORT Serial
HardwareSerial uart1(PA10, PA9); // RX, TX => D2, D8

static uint32_t roundCount = 0;

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

static bool runLoopbackOnce()
{
  const char *msg = "LOOPBACK_TEST_12345";
  const size_t len = strlen(msg);

  while (uart1.available() > 0) {
    uart1.read();
  }

  uart1.write((const uint8_t *)msg, len);
  uart1.write('\r');
  uart1.write('\n');

  char rx[64] = {0};
  size_t idx = 0;
  uint32_t start = millis();

  while ((millis() - start) < 500) {
    while (uart1.available() > 0) {
      int c = uart1.read();
      if (c < 0) {
        continue;
      }
      if (c == '\r' || c == '\n') {
        goto done;
      }
      if (idx < sizeof(rx) - 1U) {
        rx[idx++] = (char)c;
      }
    }
  }

done:
  rx[idx] = '\0';
  return strcmp(rx, msg) == 0;
}

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  PC_PORT.begin(115200);
  uart1.begin(115200);

  delay(300);
  PC_PORT.println();
  PC_PORT.println("=== UART1 Loopback Test ===");
  PC_PORT.println("Short D8(TX) <-> D2(RX)");
  PC_PORT.println("USART1: 115200 8N1");
}

void loop()
{
  roundCount++;
  bool ok = runLoopbackOnce();

  PC_PORT.print("[ROUND ");
  PC_PORT.print(roundCount);
  PC_PORT.print("] ");
  if (ok) {
    PC_PORT.println("PASS");
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
  } else {
    PC_PORT.println("FAIL");
    for (int i = 0; i < 3; i++) {
      digitalWrite(LED_BUILTIN, HIGH);
      delay(80);
      digitalWrite(LED_BUILTIN, LOW);
      delay(80);
    }
  }

  delay(1000);
}
