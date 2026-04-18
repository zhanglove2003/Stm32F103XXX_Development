#ifndef __DTH12_H__
#define __DTH12_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

#define DTH12_I2C_ADDR          0x44
#define DTH12_CMD_MEASURE       0x2C10
#define DTH12_CMD_TEMP          0xCC44
#define DTH12_CMD_HUM           0xCC66
#define DTH12_CMD_SOFT_RESET    0x30A2
#define DTH12_CMD_READ_REG      0xD2

typedef struct {
    int16_t temperature;
    uint16_t humidity;
    uint8_t error;
} DTH12_Data_t;

typedef struct {
    uint16_t HumA;
    uint16_t HumB;
    uint8_t initialized;
} DTH12_Calibration_t;

void DTH12_Init(void);
uint8_t DTH12_ReadCalibrationParams(void);
uint8_t DTH12_TriggerMeasurement(void);
uint8_t DTH12_ReadData(DTH12_Data_t *data);
uint8_t DTH12_SoftReset(void);
uint8_t DTH12_ReadRegister(uint8_t reg_addr, uint16_t *data);
uint8_t DTH12_TriggerTempMeasurement(void);
uint8_t DTH12_TriggerHumMeasurement(void);
uint8_t DTH12_ReadTempData(int16_t *temp);
uint8_t DTH12_ReadHumData(uint16_t *hum);
uint8_t DTH12_CheckCRC(uint8_t *data, uint8_t nbrOfBytes);

#ifdef __cplusplus
}
#endif

#endif
