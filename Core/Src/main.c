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
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdio.h"
#include "dth12.h"
#include "string.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* 必加：定义两个变量 */
uint32_t adc_value;        // ADC采样值（32位，与DMA对齐）
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
  // 使用DMA传输的adc_value变量，转换为16位
  uint16_t adc_val = (uint16_t)adc_value;
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
  /* USER CODE BEGIN 2 */
  int16_t temperature = 0;
  uint16_t humidity = 0;
  int16_t temp_sum = 0;
  uint16_t hum_sum = 0;
  char uart_buf[128];
  uint8_t ret;
  
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
  
  // 启动ADC和DMA
  HAL_ADC_Start_DMA(&hadc1, (uint32_t*)&adc_value, 1);
  
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
    snprintf(uart_buf, sizeof(uart_buf), "---环境参数---\r\n温度：%.1f℃\r\n湿度：%.1f%%\r\n可燃气体浓度：%dppm\r\n", temperature / 10.0f, humidity / 10.0f, gas_concentration);
    HAL_UART_Transmit(&huart1, (uint8_t*)uart_buf, strlen(uart_buf), 500);

    HAL_Delay(1000);
    /* USER CODE END 3 */
  }
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
