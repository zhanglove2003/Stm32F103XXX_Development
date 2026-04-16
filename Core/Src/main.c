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
#include "adc.h"
#include "dma.h"
#include "i2c.h"
#include "spi.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdio.h"
#include "dth12.h"
#include "string.h"
#include "st7735.h"
#include "fonts.h"
extern const uint8_t chinese_font[11][32];
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* 必加：定义变量 */
uint16_t gas_concentration; // MQ2气体浓度
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define AVG_SAMPLES 3
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* USER CODE BEGIN PV */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/**
  * @brief  读取MQ-2传感器的可燃气体浓度
  * @retval 可燃气体浓度值(ppm)
  */
uint16_t MQ2_ReadGasConcentration(void)
{
  HAL_ADC_Start(&hadc1);
  HAL_ADC_PollForConversion(&hadc1, 100);
  uint16_t adc_val = HAL_ADC_GetValue(&hadc1);
  // 将ADC值转换为可燃气体浓度
  // 这里使用一个简单的线性转换公式，实际应用中需要根据传感器特性进行校准
  // 假设ADC值范围为0-4095，对应浓度范围为0-1000ppm
  uint16_t concentration = (adc_val * 1000) / 4095;
  
  return concentration;
}

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
  MX_ADC1_Init();
  MX_I2C1_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  MX_USART3_UART_Init();
  MX_SPI1_Init();
  ST7735_Init();
  /* USER CODE BEGIN 2 */
  int16_t temperature = 0;
  uint16_t humidity = 0;
  int16_t temp_sum = 0;
  uint16_t hum_sum = 0;
  char uart_buf[128];
  uint8_t ret;
  
ST7735_FillScreen(ST7735_BLACK);

  DTH12_Init();
  
  ret = DTH12_ReadCalibrationParams();
  if (ret == 0)
  {
    printf("DTH12温湿度传感器初始化完成\r\n");
  }
  else
  {
    printf("校准参数读取失败\r\n");
  }
  
  HAL_Delay(1000);  
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    int16_t temperature = 0;
    uint16_t humidity = 0;
    int16_t temp_sum = 0;
    uint16_t hum_sum = 0;
    char uart_buf[128];
    uint8_t ret;

    temp_sum = 0;
    hum_sum = 0;

    for (uint8_t i = 0; i < AVG_SAMPLES; i++)
    {
      ret = DTH12_TriggerMeasurement();
      if (ret == 0)
      {
        ret = DTH12_ReadTempData(&temperature);
        if (ret == 0) temp_sum += temperature;

        HAL_Delay(100);

        // 使用DMA读取湿度数据
        ret = DTH12_ReadHumData_DMA(&humidity);
        if (ret == 0) hum_sum += humidity;
      }
      HAL_Delay(300);  // 这里统一延时，不乱来
    }

    temperature = temp_sum / AVG_SAMPLES;
    humidity = hum_sum / AVG_SAMPLES;

    if (humidity > 1000) humidity = 1000;
    else if (humidity < 0) humidity = 0;

    // 读取MQ-2传感器的可燃气体浓度
    gas_concentration = MQ2_ReadGasConcentration();

    // 串口输出
    snprintf(uart_buf, sizeof(uart_buf), "---环境参数---\r\n温度：%.1f℃\r\n湿度：%.1f%%\r\n烟雾浓度：%dppm\r\n", temperature / 10.0f, humidity / 10.0f, gas_concentration);
    HAL_UART_Transmit(&huart1, (uint8_t*)uart_buf, strlen(uart_buf), 500);

    // 显示屏输出环境参数
    char lcd_buf[32];
    ST7735_FillScreen(ST7735_BLACK);
    // 标题居中显示汉字
    ST7735_ShowChinese(32, 60, chinese_font[0], ST7735_WHITE, ST7735_BLACK);  // 环
    ST7735_ShowChinese(48, 60, chinese_font[1], ST7735_WHITE, ST7735_BLACK);  // 境
    ST7735_ShowChinese(64, 60, chinese_font[2], ST7735_WHITE, ST7735_BLACK);  // 参
    ST7735_ShowChinese(80, 60, chinese_font[3], ST7735_WHITE, ST7735_BLACK);  // 数
    // 温度
    ST7735_ShowChinese(0, 40, chinese_font[4], ST7735_WHITE, ST7735_BLACK);  // 温
    ST7735_ShowChinese(16, 40, chinese_font[6], ST7735_WHITE, ST7735_BLACK); // 度
    snprintf(lcd_buf, sizeof(lcd_buf), ":%.1fC", temperature / 10.0f);
    ST7735_DrawString(32, 40, lcd_buf, ST7735_WHITE, ST7735_BLACK, &Font_7x10);
    // 湿度
    ST7735_ShowChinese(0, 20, chinese_font[5], ST7735_WHITE, ST7735_BLACK);  // 湿
    ST7735_ShowChinese(16, 20, chinese_font[6], ST7735_WHITE, ST7735_BLACK); // 度
    snprintf(lcd_buf, sizeof(lcd_buf), ":%.1f%%", humidity / 10.0f);
    ST7735_DrawString(32, 20, lcd_buf, ST7735_WHITE, ST7735_BLACK, &Font_7x10);
    // 烟雾浓度
    ST7735_ShowChinese(0, 0, chinese_font[7], ST7735_WHITE, ST7735_BLACK);  // 烟
    ST7735_ShowChinese(16, 0, chinese_font[8], ST7735_WHITE, ST7735_BLACK); // 雾
    ST7735_ShowChinese(32, 0, chinese_font[9], ST7735_WHITE, ST7735_BLACK); // 浓
    ST7735_ShowChinese(48, 0, chinese_font[10], ST7735_WHITE, ST7735_BLACK); // 度
    snprintf(lcd_buf, sizeof(lcd_buf), ":%dppm", gas_concentration);
    ST7735_DrawString(64, 0, lcd_buf, ST7735_WHITE, ST7735_BLACK, &Font_7x10);

    HAL_Delay(1000);
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
void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    // 判断是不是我们用的那个 I2C (比如 I2C1)
    if (hi2c->Instance == I2C1)
    {
        // 1. 告诉主程序：数据我搬运完了！
        dht12_read_complete = 1;
    }
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
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
