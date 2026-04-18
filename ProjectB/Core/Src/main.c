/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dma.h"
#include "spi.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "st7735.h"
#include "fonts.h"

extern const uint8_t chinese_font[11][32];
extern const unsigned char gImage_初始化加载[];
extern const unsigned char gImage_环境参数[];
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define RX_BUF_SIZE 128
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  MX_SPI1_Init();
  /* USER CODE BEGIN 2 */
  uint8_t rx_data[1];
  uint8_t rx_buffer[RX_BUF_SIZE];
  uint16_t rx_index = 0;
  float current_temp = 0, current_hum = 0;
  int current_gas = 0;
  char lcd_buf[32];

  printf("System Start\r\n");

  ST7735_Init();
  ST7735_FillScreen(ST7735_BLACK);

  ST7735_DrawImage(0, 0, 80, 79, gImage_初始化加载 + 8);
  ST7735_DrawString(84, 50, "Make", ST7735_WHITE, ST7735_BLACK, &Font_11x18);
  ST7735_DrawString(84, 30, "By", ST7735_WHITE, ST7735_BLACK, &Font_11x18);
  ST7735_DrawString(84, 10, "Snow", ST7735_WHITE, ST7735_BLACK, &Font_11x18);

  HAL_Delay(5000);

  HAL_GPIO_WritePin(MD0_GPIO_Port, MD0_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(MD1_GPIO_Port, MD1_Pin, GPIO_PIN_RESET);

  HAL_Delay(100);

  printf("Ready\r\n");

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    if (HAL_UART_Receive(&huart2, rx_data, 1, 10) == HAL_OK)
    {
      if (rx_index < RX_BUF_SIZE - 1)
      {
        rx_buffer[rx_index++] = rx_data[0];

        if (rx_data[0] == '\n' || rx_data[0] == '\r')
        {
          rx_buffer[rx_index] = '\0';

          if (rx_index > 2)
          {
            float temp = 0, hum = 0;
            int gas = 0;

            char *t_ptr = strstr((char*)rx_buffer, "T");
            char *h_ptr = strstr((char*)rx_buffer, "H");
            char *s_ptr = strstr((char*)rx_buffer, "S");

            if (t_ptr && h_ptr)
            {
              temp = atof(t_ptr + 1);
              hum = atof(h_ptr + 1);

              if (s_ptr)
              {
                gas = atoi(s_ptr + 1);
              }

              current_temp = temp;
              current_hum = hum;
              current_gas = gas;

              ST7735_FillScreen(ST7735_BLACK);
              ST7735_ShowChinese(15, 60, chinese_font[0], ST7735_WHITE, ST7735_BLACK);
              ST7735_ShowChinese(35, 60, chinese_font[1], ST7735_WHITE, ST7735_BLACK);
              ST7735_ShowChinese(55, 60, chinese_font[2], ST7735_WHITE, ST7735_BLACK);
              ST7735_ShowChinese(75, 60, chinese_font[3], ST7735_WHITE, ST7735_BLACK);

              ST7735_ShowChinese(0, 40, chinese_font[4], ST7735_WHITE, ST7735_BLACK);
              ST7735_ShowChinese(16, 40, chinese_font[6], ST7735_WHITE, ST7735_BLACK);
              snprintf(lcd_buf, sizeof(lcd_buf), ":%.1fC", temp);
              ST7735_DrawString(32, 40, lcd_buf, ST7735_WHITE, ST7735_BLACK, &Font_7x10);

              ST7735_ShowChinese(0, 20, chinese_font[5], ST7735_WHITE, ST7735_BLACK);
              ST7735_ShowChinese(16, 20, chinese_font[6], ST7735_WHITE, ST7735_BLACK);
              snprintf(lcd_buf, sizeof(lcd_buf), ":%.1f%%", hum);
              ST7735_DrawString(32, 20, lcd_buf, ST7735_WHITE, ST7735_BLACK, &Font_7x10);

              ST7735_ShowChinese(0, 0, chinese_font[7], ST7735_WHITE, ST7735_BLACK);
              ST7735_ShowChinese(16, 0, chinese_font[8], ST7735_WHITE, ST7735_BLACK);
              ST7735_ShowChinese(32, 0, chinese_font[9], ST7735_WHITE, ST7735_BLACK);
              ST7735_ShowChinese(48, 0, chinese_font[10], ST7735_WHITE, ST7735_BLACK);
              snprintf(lcd_buf, sizeof(lcd_buf), ":%dppm", gas);
              ST7735_DrawString(64, 0, lcd_buf, ST7735_WHITE, ST7735_BLACK, &Font_7x10);

              ST7735_DrawImage(95, 20, 60, 60, gImage_环境参数 + 8);
            }
            else
            {
            }
          }

          rx_index = 0;
          memset(rx_buffer, 0, RX_BUF_SIZE);
        }
      }
      else
      {
        rx_index = 0;
        memset(rx_buffer, 0, RX_BUF_SIZE);
      }
    }
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
/* USER CODE END 4 */

/**
  * @brief  This function executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error error occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
