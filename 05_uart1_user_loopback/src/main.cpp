#include <Arduino.h>

#define PC_PORT Serial
HardwareSerial uart1(PA10, PA9); // RX=D2, TX=D8

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

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  PC_PORT.begin(115200);
  uart1.begin(115200);

  delay(300);
  PC_PORT.println();
  PC_PORT.println("=== UART1 User Loopback ===");
  PC_PORT.println("Short D8(TX) <-> D2(RX)");
  PC_PORT.println("Type in COM13, data will echo back from UART1");
  PC_PORT.print("> ");
}

void loop()
{
  while (PC_PORT.available() > 0) {
    int c = PC_PORT.read();
    if (c >= 0) {
      uart1.write((uint8_t)c);
      digitalWrite(LED_BUILTIN, HIGH);
    }
  }

  while (uart1.available() > 0) {
    int c = uart1.read();
    if (c >= 0) {
      PC_PORT.write((uint8_t)c);
      digitalWrite(LED_BUILTIN, LOW);
    }
  }
}
