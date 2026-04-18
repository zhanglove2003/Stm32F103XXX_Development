#ifndef __ESP8266_H__
#define __ESP8266_H__

#include "main.h"
#include "usart.h"
#include "string.h"
#include "stdio.h"

#define ESP8266_WIFI_SSID "WFIF003"
#define ESP8266_WIFI_PASS "1234567890"

#define ESP8266_MQTT_BROKER "iot-06z00hqfmjy48dl.mqtt.iothub.aliyuncs.com"
#define ESP8266_MQTT_PORT 1883
#define ESP8266_MQTT_TOPIC "/sys/k1i5905VVGW/SnowWiFi/thing/event/property/post"

typedef enum {
    ESP_OK = 0,
    ESP_ERROR = 1,
    ESP_TIMEOUT = 2
} ESP_Status;

typedef struct {
    char wifi_ssid[64];
    char wifi_pass[64];
    char mqtt_broker[128];
    uint16_t mqtt_port;
    char mqtt_client_id[256];
    char mqtt_username[64];
    char mqtt_password[128];
} ESP8266_Config;

void ESP8266_ShowConfigMenu(void);
uint8_t ESP8266_GetUserChoice(void);
void ESP8266_ConfigureWiFi(ESP8266_Config *config);
void ESP8266_ConfigureMQTT(ESP8266_Config *config);
void ESP8266_ParseAlibabaCloudParams(ESP8266_Config *config, char *input);
ESP_Status ESP8266_InitModule(void);
ESP_Status ESP8266_ConnectToNetwork(ESP8266_Config *config);
ESP_Status ESP8266_PublishTemperature(float temp);
ESP_Status ESP8266_PublishHumidity(float hum);
ESP_Status ESP8266_PublishGas(int gas);

#endif
