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
    // dth12_cal.HumA = 0;
    // dth12_cal.HumB = 0;
    // dth12_cal.initialized = 0;
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

uint8_t DTH12_ReadHumData(uint16_t *hum) {
    uint8_t read_buf[3] = {0};
    uint16_t raw_data = 0;
    uint16_t humidity_x10 = 0;
    float temp_hum = 0.0f; // 定义一个浮点数变量用于中间计算

    // 1. 读取数据
    if (HAL_I2C_Master_Receive(&hi2c1, DTH12_I2C_ADDR << 1, read_buf, 3, 1000) != HAL_OK) {
        return 1; // 读取失败
    }

    // 2. 组合原始数据
    raw_data = (read_buf[0] << 8) | read_buf[1];

    // 3. 屏蔽无效位 (DHT12 湿度是 14位精度)
    raw_data = raw_data & 0x3FFF;

    // 4. 计算湿度 (使用浮点数计算更精准)
    // 公式：(原始数据 / 16384) * 100
    temp_hum = (float)raw_data * 100.0f / 16384.0f;

    // 5. 【关键步骤】手动校准偏差
    // 你的读数偏高约 20%，这里减去 20.0 进行修正
    // 如果减去后觉得太低，可以把 20.0 改成 15.0 或 10.0
    temp_hum = temp_hum - 20.0f;

    // 6. 防止修正后变成负数
    if (temp_hum < 0) {
        temp_hum = 0;
    }

    // 7. 转换为整数格式 (乘以 10 是为了保留一位小数，例如 62.5%)
    humidity_x10 = (uint16_t)(temp_hum * 10.0f);

    // 8. 限制范围 (防止溢出)
    if (humidity_x10 > 1000) {
        humidity_x10 = 1000;
    }

    // 9. 输出结果
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
