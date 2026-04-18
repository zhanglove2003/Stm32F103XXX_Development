#include "esp8266.h"

#include "stdlib.h"
#include "string.h"
#include "stdio.h"
#include "main.h"
#include "usart.h"

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart3;

static char esp_rx_buffer[512];
static uint16_t esp_rx_index = 0;

static void ESP8266_ClearBuffer(void)
{
    memset(esp_rx_buffer, 0, sizeof(esp_rx_buffer));
    esp_rx_index = 0;
}

static void ESP8266_ReadResponse(uint32_t timeout)
{
    uint8_t ch;
    uint32_t start_time = HAL_GetTick();
    
    ESP8266_ClearBuffer();
    
    while(HAL_GetTick() - start_time < timeout)
    {
        if(HAL_UART_Receive(&huart3, &ch, 1, 10) == HAL_OK)
        {
            if(esp_rx_index < sizeof(esp_rx_buffer) - 1)
            {
                esp_rx_buffer[esp_rx_index++] = ch;
                start_time = HAL_GetTick();
            }
        }
    }
    esp_rx_buffer[esp_rx_index] = '\0';
}

void ESP8266_ShowConfigMenu(void)
{
    printf("\r\n=== ESP8266 Config ===\r\n");
    printf("[1] WiFi\r\n");
    printf("[2] MQTT\r\n");
    printf("[3] Run\r\n");
}

uint8_t ESP8266_GetUserChoice(void)
{
    uint8_t choice[2] = {0};
    uint32_t timeout_start = HAL_GetTick();
    
    while(HAL_GetTick() - timeout_start < 30000)
    {
        if(HAL_UART_Receive(&huart1, choice, 1, 100) == HAL_OK)
        {
            if(choice[0] >= '1' && choice[0] <= '3')
            {
                return choice[0] - '0';
            }
        }
    }
    return 3;
}

void ESP8266_ConfigureWiFi(ESP8266_Config *config)
{
    char input[64];
    uint8_t ch;
    int index = 0;
    
    printf("WiFi SSID: ");
    index = 0;
    while(1)
    {
        if(HAL_UART_Receive(&huart1, &ch, 1, 500) == HAL_OK)
        {
            if(ch == '\r' || ch == '\n')
            {
                input[index] = '\0';
                break;
            }
            else if(ch == 0x08 && index > 0)
            {
                index--;
            }
            else if(index < (int)sizeof(input) - 1 && ch >= 0x20)
            {
                input[index++] = ch;
            }
        }
    }
    if(index > 0) strcpy(config->wifi_ssid, input);
    
    printf("WiFi PASS: ");
    index = 0;
    while(1)
    {
        if(HAL_UART_Receive(&huart1, &ch, 1, 500) == HAL_OK)
        {
            if(ch == '\r' || ch == '\n')
            {
                input[index] = '\0';
                break;
            }
            else if(ch == 0x08 && index > 0)
            {
                index--;
            }
            else if(index < (int)sizeof(input) - 1 && ch >= 0x20)
            {
                input[index++] = ch;
            }
        }
    }
    if(index > 0) strcpy(config->wifi_pass, input);
    printf("OK\r\n");
}

void ESP8266_ParseAlibabaCloudParams(ESP8266_Config *config, char *input)
{
    char *p, *v;
    int len;
    
    p = strstr(input, "clientId");
    if(p)
    {
        p = strchr(p, '\n');
        if(p)
        {
            p++;
            v = strchr(p, '\n');
            if(v)
            {
                len = v - p;
                if(len > 0 && len < 255)
                {
                    memcpy(config->mqtt_client_id, p, len);
                    config->mqtt_client_id[len] = '\0';
                    while(len > 0 && config->mqtt_client_id[len-1] <= ' ')
                        config->mqtt_client_id[--len] = '\0';
                }
            }
        }
    }
    
    p = strstr(input, "username");
    if(p)
    {
        p = strchr(p, '\n');
        if(p)
        {
            p++;
            v = strchr(p, '\n');
            if(v)
            {
                len = v - p;
                if(len > 0 && len < 63)
                {
                    memcpy(config->mqtt_username, p, len);
                    config->mqtt_username[len] = '\0';
                    while(len > 0 && config->mqtt_username[len-1] <= ' ')
                        config->mqtt_username[--len] = '\0';
                }
            }
        }
    }
    
    p = strstr(input, "passwd");
    if(p)
    {
        p = strchr(p, '\n');
        if(p)
        {
            p++;
            v = strchr(p, '\n');
            if(v)
            {
                len = v - p;
                if(len > 0 && len < 127)
                {
                    memcpy(config->mqtt_password, p, len);
                    config->mqtt_password[len] = '\0';
                    while(len > 0 && config->mqtt_password[len-1] <= ' ')
                        config->mqtt_password[--len] = '\0';
                }
            }
        }
    }
    
    p = strstr(input, "mqttHostUrl");
    if(p)
    {
        p = strchr(p, '\n');
        if(p)
        {
            p++;
            v = strchr(p, '\n');
            if(v)
            {
                len = v - p;
                if(len > 0 && len < 127)
                {
                    memcpy(config->mqtt_broker, p, len);
                    config->mqtt_broker[len] = '\0';
                    while(len > 0 && config->mqtt_broker[len-1] <= ' ')
                        config->mqtt_broker[--len] = '\0';
                }
            }
        }
    }
    
    p = strstr(input, "\nport\n");
    if(p)
    {
        p += 5;
        v = strchr(p, '\n');
        if(v)
        {
            char port_str[10];
            len = v - p;
            if(len > 0 && len < 9)
            {
                memcpy(port_str, p, len);
                port_str[len] = '\0';
                config->mqtt_port = atoi(port_str);
            }
        }
    }
}

void ESP8266_ConfigureMQTT(ESP8266_Config *config)
{
    char input[512];
    uint8_t ch;
    int index = 0;
    
    printf("\r\n[Paste Alibaba Cloud params]\r\n");
    printf("clientId/username/passwd/mqttHostUrl/port\r\n");
    printf("> ");
    
    index = 0;
    memset(input, 0, sizeof(input));
    
    uint32_t t0 = HAL_GetTick();
    while(HAL_GetTick() - t0 < 60000)
    {
        if(HAL_UART_Receive(&huart1, &ch, 1, 50) == HAL_OK)
        {
            if(ch == '\r' || ch == '\n')
            {
                if(index > 20)
                {
                    input[index] = '\0';
                    break;
                }
            }
            else if(ch == 0x08 && index > 0)
            {
                index--;
            }
            else if(index < 511 && ch >= 0x20)
            {
                input[index++] = ch;
            }
            t0 = HAL_GetTick();
        }
    }
    
    if(index > 20)
    {
        ESP8266_ParseAlibabaCloudParams(config, input);
        printf("\r\nParsed OK\r\n");
    }
    else
    {
        printf("\r\nTimeout\r\n");
    }
}

ESP_Status ESP8266_InitModule(void)
{
    HAL_Delay(1000);
    
    HAL_UART_Transmit(&huart3, (uint8_t*)"AT+RST\r\n", 7, 1000);
    HAL_Delay(4000);
    
    HAL_UART_Transmit(&huart3, (uint8_t*)"AT+CWMODE=1\r\n", 12, 1000);
    HAL_Delay(500);
    
    HAL_UART_Transmit(&huart3, (uint8_t*)"AT+CIPMUX=0\r\n", 11, 1000);
    HAL_Delay(500);
    
    return ESP_OK;
}

ESP_Status ESP8266_ConnectToNetwork(ESP8266_Config *config)
{
    char cmd[256];
    
    snprintf(cmd, sizeof(cmd), "AT+CWJAP=\"%s\",\"%s\"\r\n", config->wifi_ssid, config->wifi_pass);
    HAL_UART_Transmit(&huart3, (uint8_t*)cmd, strlen(cmd), 1000);
    printf("WiFi...");
    for(int i = 0; i < 15; i++) { HAL_Delay(1000); }
    
    snprintf(cmd, sizeof(cmd), "AT+MQTTUSERCFG=0,1,\"NULL\",\"%s\",\"%s\",0,0,\"\"\r\n",
             config->mqtt_username, config->mqtt_password);
    HAL_UART_Transmit(&huart3, (uint8_t*)cmd, strlen(cmd), 1000);
    HAL_Delay(1000);
    
    snprintf(cmd, sizeof(cmd), "AT+MQTTCLIENTID=0,\"%s\"\r\n", config->mqtt_client_id);
    HAL_UART_Transmit(&huart3, (uint8_t*)cmd, strlen(cmd), 1000);
    HAL_Delay(1000);
    
    snprintf(cmd, sizeof(cmd), "AT+MQTTCONN=0,\"%s\",%d,1\r\n",
             config->mqtt_broker, config->mqtt_port);
    HAL_UART_Transmit(&huart3, (uint8_t*)cmd, strlen(cmd), 1000);
    printf("MQTT...");
    for(int i = 0; i < 15; i++) { HAL_Delay(1000); }
    
    return ESP_OK;
}

ESP_Status ESP8266_PublishTemperature(float temp)
{
    char cmd[256], msg[64];
    snprintf(msg, sizeof(msg), "{\"id\":\"123\",\"params\":{\"EnvTemperature\":%.1f}}", temp);
    snprintf(cmd, sizeof(cmd), "AT+MQTTPUB=0,\"%s\",\"%s\",1,0\r\n", ESP8266_MQTT_TOPIC, msg);
    HAL_UART_Transmit(&huart3, (uint8_t*)cmd, strlen(cmd), 1000);
    HAL_Delay(200);
    return ESP_OK;
}

ESP_Status ESP8266_PublishHumidity(float hum)
{
    char cmd[256], msg[64];
    snprintf(msg, sizeof(msg), "{\"id\":\"123\",\"params\":{\"EnvHumidity\":%.1f}}", hum);
    snprintf(cmd, sizeof(cmd), "AT+MQTTPUB=0,\"%s\",\"%s\",1,0\r\n", ESP8266_MQTT_TOPIC, msg);
    HAL_UART_Transmit(&huart3, (uint8_t*)cmd, strlen(cmd), 1000);
    HAL_Delay(200);
    return ESP_OK;
}

ESP_Status ESP8266_PublishGas(int gas)
{
    char cmd[256], msg[64];
    snprintf(msg, sizeof(msg), "{\"id\":\"123\",\"params\":{\"GasConcentration\":%d}}", gas);
    snprintf(cmd, sizeof(cmd), "AT+MQTTPUB=0,\"%s\",\"%s\",1,0\r\n", ESP8266_MQTT_TOPIC, msg);
    HAL_UART_Transmit(&huart3, (uint8_t*)cmd, strlen(cmd), 1000);
    HAL_Delay(200);
    return ESP_OK;
}
