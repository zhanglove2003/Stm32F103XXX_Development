#ifndef __ESP8266_H__
#define __ESP8266_H__

#include "main.h"
#include "usart.h"

uint8_t ESP8266_Init(void);
void ESP8266_PublishData(float temp, float hum, int gas);

#endif
