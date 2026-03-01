/*
 * 將以下內容貼進 CubeIDE 產生的 Core/Src/main.c 對應區塊
 * 板子: NUCLEO-L053R8
 * UART: USART2, 115200-8-N-1
 */

/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
/* USER CODE END Includes */

/* USER CODE BEGIN PV */
extern UART_HandleTypeDef huart2;
static uint32_t g_count = 0;
/* USER CODE END PV */

/* USER CODE BEGIN 0 */
int _write(int file, char *ptr, int len)
{
  (void)file;
  HAL_UART_Transmit(&huart2, (uint8_t*)ptr, (uint16_t)len, HAL_MAX_DELAY);
  return len;
}
/* USER CODE END 0 */

/* USER CODE BEGIN 2 */
printf("boot ok\\r\\n");
/* USER CODE END 2 */

/* USER CODE BEGIN WHILE */
while (1)
{
  g_count++;
  printf("hello world %lu\\r\\n", g_count);
  HAL_Delay(1000);

  /* USER CODE END WHILE */

  /* USER CODE BEGIN 3 */
}
/* USER CODE END 3 */
