#include "dth12.h"
#include "i2c.h"
#include <string.h>

static DTH12_Calibration_t dth12_cal = {0};


const uint16_t POLYNOMIAL = 0x131;

void DTH12_Init(void)
{
    uint8_t cmd[2] = {0x30, 0xA2};
    
    HAL_I2C_Master_Transmit(&hi2c1, DTH12_I2C_ADDR << 1, cmd, 2, 1000);
    HAL_Delay(10);
    
    DTH12_ReadCalibrationParams();
    dth12_cal.initialized = 0;
}

uint8_t DTH12_TriggerMeasurement(void)
{
    uint8_t cmd[2] = {0x2C, 0x10};
    
    if (HAL_I2C_Master_Transmit(&hi2c1, DTH12_I2C_ADDR << 1, cmd, 2, 1000) != HAL_OK)
    {
        return 1;
    }
    
    HAL_Delay(100);
    return 0;
}

uint8_t DTH12_ReadTempData(int16_t *temp)
{
    uint8_t read_buf[3] = {0};
    
    if (HAL_I2C_Master_Receive(&hi2c1, DTH12_I2C_ADDR << 1, read_buf, 3, 1000) != HAL_OK)
    {
        return 1;
    }
    
    int16_t temp_raw = (read_buf[0] << 8) | read_buf[1];
    *temp = 400 + temp_raw / 25.6;
    
    return 0;
}


uint8_t DTH12_Start_DMA_Read(void) {
    // 1. 重置完成标志位
    dht12_read_complete = 0;

    // 2. 启动 I2C DMA 接收
    if (HAL_I2C_Master_Receive_DMA(&hi2c1, DTH12_I2C_ADDR << 1, dht12_dma_rx_buf, 3) != HAL_OK) {
        return 1; // 启动失败
    }

    return 0; // 启动成功
}

// 这是一个处理函数，用来从全局缓冲区提取数据
uint8_t DTH12_Process_Data(uint16_t *hum) {
    // 如果还没读完，直接返回
    if (dht12_read_complete == 0) {
        return 1;
    }

    uint16_t raw_data = 0;
    float temp_hum = 0.0f;
    uint16_t humidity_x10 = 0;

    // 1. 组合原始数据 (从 DMA 缓冲区读取)
    raw_data = (dht12_dma_rx_buf[0] << 8) | dht12_dma_rx_buf[1];

    // 2. 屏蔽无效位
    raw_data = raw_data & 0x3FFF;

    // 3. 计算湿度 (带校准)
    temp_hum = (float)raw_data * 100.0f / 16384.0f;
    temp_hum = temp_hum - 20.0f; // 校准偏移量
    if (temp_hum < 0) temp_hum = 0;

    humidity_x10 = (uint16_t)(temp_hum * 10);

    *hum = humidity_x10;
    return 0;
}

uint8_t DTH12_ReadData(DTH12_Data_t *data)
{
    uint8_t read_buf[6] = {0};
    
    if (HAL_I2C_Master_Receive(&hi2c1, DTH12_I2C_ADDR << 1, read_buf, 6, 1000) != HAL_OK)
    {
        data->error = 1;
        return 1;
    }
    
    int16_t temp_raw = (read_buf[0] << 8) | read_buf[1];
    int16_t hum_raw = (read_buf[3] << 8) | read_buf[4];
    
    data->temperature = 400 + temp_raw / 25.6;
    
    if (dth12_cal.initialized)
    {
        data->humidity = (hum_raw - dth12_cal.HumB) * 600 / (dth12_cal.HumA - dth12_cal.HumB) + 300;
    }
    else
    {
        data->humidity = hum_raw;
    }
    
    if (data->humidity > 1000)
        data->humidity = 1000;
    else if (data->humidity < 0)
        data->humidity = 0;
    
    data->error = 0;
    return 0;
}

uint8_t DTH12_SoftReset(void)
{
    uint8_t cmd[2] = {0x30, 0xA2};
    
    if (HAL_I2C_Master_Transmit(&hi2c1, DTH12_I2C_ADDR << 1, cmd, 2, 1000) != HAL_OK)
    {
        return 1;
    }
    
    HAL_Delay(10);
    return 0;
}

uint8_t DTH12_ReadRegister(uint8_t reg_addr, uint16_t *data)
{
    uint8_t cmd[2] = {0xD2, reg_addr};
    uint8_t read_buf[3] = {0};
    
    if (HAL_I2C_Master_Transmit(&hi2c1, DTH12_I2C_ADDR << 1, cmd, 2, 1000) != HAL_OK)
    {
        return 1;
    }
    
    HAL_Delay(10);
    
    if (HAL_I2C_Master_Receive(&hi2c1, DTH12_I2C_ADDR << 1, read_buf, 3, 1000) != HAL_OK)
    {
        return 1;
    }
    
    *data = (read_buf[0] << 8) | read_buf[1];
    return 0;
}

uint8_t DTH12_ReadCalibrationParams(void)
{
    uint16_t data = 0;
    
    if (DTH12_ReadRegister(8, &data) != 0)
    {
        return 1;
    }
    dth12_cal.HumA = data;
    
    if (DTH12_ReadRegister(10, &data) != 0)
    {
        return 1;
    }
    dth12_cal.HumB = data;
    
    dth12_cal.initialized = 1;
    return 0;
}

uint8_t DTH12_TriggerTempMeasurement(void)
{
    return DTH12_TriggerMeasurement();
}

uint8_t DTH12_TriggerHumMeasurement(void)
{
    return DTH12_TriggerMeasurement();
}

uint8_t DTH12_CheckCRC(uint8_t *data, uint8_t nbrOfBytes)
{
    uint8_t crc = 0xFF;
    uint8_t byteCtr, bit;
    
    for (byteCtr = 0; byteCtr < nbrOfBytes; ++byteCtr)
    {
        crc ^= (data[byteCtr]);
        for (bit = 8; bit > 0; --bit)
        {
            if (crc & 0x80)
                crc = (crc << 1) ^ POLYNOMIAL;
            else
                crc = (crc << 1);
        }
    }
    
    if (crc != data[nbrOfBytes])
    {
        return 1;
    }
    else
        return 0;
}
uint8_t dht12_dma_rx_buf[3] = {0};

// 定义一个标志位，用来判断数据是否读取完成
volatile uint8_t dht12_read_complete = 0;

uint8_t DTH12_ReadHumData(uint16_t *hum)
{
    uint8_t read_buf[3] = {0};
    
    if (HAL_I2C_Master_Receive(&hi2c1, DTH12_I2C_ADDR << 1, read_buf, 3, 1000) != HAL_OK)
    {
        return 1;
    }
    
    uint16_t hum_raw = (read_buf[0] << 8) | read_buf[1];
    
    if (dth12_cal.initialized)
    {
        *hum = (hum_raw - dth12_cal.HumB) * 600 / (dth12_cal.HumA - dth12_cal.HumB) + 300;
    }
    else
    {
        *hum = hum_raw;
    }
    
    if (*hum > 1000)
        *hum = 1000;
    else if (*hum < 0)
        *hum = 0;
    
    return 0;
}

uint8_t DTH12_ReadHumData_DMA(uint16_t *hum)
{
    // 启动DMA读取
    if (DTH12_Start_DMA_Read() != 0)
    {
        return 1;
    }
    
    // 等待读取完成
    uint32_t timeout = 1000;
    while (dht12_read_complete == 0 && timeout > 0)
    {
        HAL_Delay(1);//不延迟就会读不到湿度
        timeout--;
    }
    
    if (timeout == 0)
    {
        return 1;
    }
    
    // 处理数据
    return DTH12_Process_Data(hum);
}


