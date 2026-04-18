#include "esp8266.h"
#include <string.h>
#include <stdio.h>

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart3;

static uint8_t esp8266_connected = 0;

static void ESP8266_SendCmd(const char *cmd)
{
    HAL_UART_Transmit(&huart3, (uint8_t*)cmd, strlen(cmd), 1000);
    printf("ESP8266: TX: %s", cmd);
    HAL_Delay(300);
}

static void ESP8266_ClearBuffer(void)
{
    uint8_t ch;
    while (HAL_UART_Receive(&huart3, &ch, 1, 10) == HAL_OK);
}

static int ESP8266_WaitForResponse(const char *success, uint32_t timeout_ms)
{
    uint32_t start = HAL_GetTick();
    char buffer[256];
    size_t blen = 0;

    while (HAL_GetTick() - start < timeout_ms)
    {
        uint8_t ch;
        if (HAL_UART_Receive(&huart3, &ch, 1, 100) == HAL_OK)
        {
            if (blen < sizeof(buffer) - 1) buffer[blen++] = ch;
            buffer[blen] = '\0';

            if (strstr(buffer, "ERROR") != NULL)
            {
                printf("ESP8266: RX ERROR\r\n");
                return -1;
            }
            if (success && strstr(buffer, success) != NULL)
            {
                printf("ESP8266: RX OK\r\n");
                return 1;
            }

            if (blen >= sizeof(buffer) - 1)
            {
                memmove(buffer, buffer + blen/2, blen/2);
                blen = blen/2;
            }
        }
    }
    return 0;
}

uint8_t ESP8266_Init(void)
{
    printf("ESP8266: Starting...\r\n");

    ESP8266_ClearBuffer();
    HAL_Delay(1000);

    ESP8266_SendCmd("AT+RST\r\n");
    HAL_Delay(4000);

    ESP8266_ClearBuffer();
    ESP8266_SendCmd("AT+MQTTUSERCFG=0,1,\"NULL\",\"SnowWiFi&k1i5905VVGW\",\"dcc97adbec1b045d92226556a367a4b5b34ff4cbb49f30e51e252c1af903905c\",0,0,\"\"\r\n");
    HAL_Delay(2000);

    ESP8266_ClearBuffer();
    ESP8266_SendCmd("AT+MQTTCLIENTID=0,\"k1i5905VVGW.SnowWiFi|securemode=2\\,signmethod=hmacsha256\\,timestamp=1776479622131|\"\r\n");
    HAL_Delay(2000);

    ESP8266_ClearBuffer();
    ESP8266_SendCmd("AT+MQTTCONN=0,\"iot-06z00hqfmjy48dl.mqtt.iothub.aliyuncs.com\",1883,1\r\n");
    HAL_Delay(5000);

    printf("ESP8266: Init done!\r\n");
    esp8266_connected = 1;
    return 1;
}

void ESP8266_PublishData(float temp, float hum, int gas)
{
    char cmd[300];

    if(!esp8266_connected)
    {
        printf("ESP8266: Not connected\r\n");
        return;
    }

    snprintf(cmd, sizeof(cmd),
             "AT+MQTTPUB=0,\"/sys/k1i5905VVGW/SnowWiFi/thing/event/property/post\",\"{\\\"id\\\":\\\"123\\\"\\,\\\"version\\\":\\\"1.0\\\"\\,\\\"params\\\":{\\\"EnvTemperature\\\":%.1f}}\",1,0\r\n",
             temp);
    ESP8266_SendCmd(cmd);
    HAL_Delay(500);

    snprintf(cmd, sizeof(cmd),
             "AT+MQTTPUB=0,\"/sys/k1i5905VVGW/SnowWiFi/thing/event/property/post\",\"{\\\"id\\\":\\\"123\\\"\\,\\\"version\\\":\\\"1.0\\\"\\,\\\"params\\\":{\\\"EnvHumidity\\\":%.1f}}\",1,0\r\n",
             hum);
    ESP8266_SendCmd(cmd);
    HAL_Delay(500);

    snprintf(cmd, sizeof(cmd),
             "AT+MQTTPUB=0,\"/sys/k1i5905VVGW/SnowWiFi/thing/event/property/post\",\"{\\\"id\\\":\\\"123\\\"\\,\\\"version\\\":\\\"1.0\\\"\\,\\\"params\\\":{\\\"GasConcentration\\\":%d}}\",1,0\r\n",
             gas);
    ESP8266_SendCmd(cmd);
    HAL_Delay(500);

    printf("ESP8266: Data sent\r\n");
}
