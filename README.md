[README.md](https://github.com/user-attachments/files/26855317/README.md)# 基于STM32的无线环境监测系统

## 项目概述

本项目是一个基于STM32F103C8微控制器的无线环境监测系统，包含**发送端(ProjectA)**和**接收端(ProjectB)**两个独立模块。系统通过DHT12传感器采集温度和湿度，通过MQ2烟雾传感器采集烟雾浓度，数据通过A39C无线模块在两模块间传输，同时发送端通过ESP8266模块将数据上传至阿里云物联网平台，接收端在ST7735液晶屏幕上实时显示环境参数。

---

## 硬件架构

### 主要硬件组件

| 组件 | 型号/规格 | 说明 |
|------|----------|------|
| 主控芯片 | STM32F103C8T6 | ARM Cortex-M3内核，72MHz主频 |
| 温湿度传感器 | DHT12 | I2C接口，精度±0.5℃/±2%RH |
| 烟雾传感器 | MQ2 | 模拟输出，ADC采集 |
| 无线模块 | A39C | 433MHz无线串口，透明传输 |
| WiFi模块 | ESP8266 | AT指令控制，MQTT协议上云 |
| 显示模块 | ST7735 | 1.8寸彩色液晶，SPI接口 |

### 硬件连接

#### ProjectA（发送端）引脚分配

| 功能 | 引脚 | 外设 |
|------|------|------|
| 调试串口(UART1) | PA9(TX), PA10(RX) | USART1 |
| A39C无线串口(UART2) | PA2(TX), PA3(RX) | USART2 |
| ESP8266串口(UART3) | PB10(TX), PB11(RX) | USART3 |
| DHT12 I2C | PB6(SCL), PB7(SDA) | I2C1 |
| MQ2模拟信号 | PA0 | ADC1_CH0 |
| A39C配置(MD0) | PA1 | GPIO_Output |
| A39C配置(MD1) | PA4 | GPIO_Output |

#### ProjectB（接收端）引脚分配

| 功能 | 引脚 | 外设 |
|------|------|------|
| 调试串口(UART1) | PA9(TX), PA10(RX) | USART1 |
| A39C无线串口(UART2) | PA2(TX), PA3(RX) | USART2 |
| ST7735 SPI | PA5(SCLK), PA7(MOSI) | SPI1 |
| ST7735 CS | PA4 | GPIO_Output |
| ST7735 DC | PB1 | GPIO_Output |
| ST7735 RES | PB0 | GPIO_Output |
| A39C配置(MD0) | PA1 | GPIO_Output |
| A39C配置(MD1) | PA4 | GPIO_Output |

---

## 软件架构

### ProjectA - 发送端

#### 功能概述

- 环境数据采集
- 数据打包与无线发送
- 阿里云MQTT上传

#### 工作流程

```
初始化
├─ 系统时钟配置（72MHz，HSE外部晶振）
├─ 外设初始化（GPIO、UART、I2C、ADC、DMA）
├─ A39C配置（MD0=高电平，MD1=低电平）
├─ DHT12传感器初始化与校准
└─ ESP8266初始化与MQTT连接

主循环
├─ 触发DHT12测量，等待2.5秒
├─ 读取温湿度数据
├─ 读取MQ2烟雾浓度（ADC采集）
├─ 数据格式化为："T%.1fH%.1fS%d\r\n"
├─ 通过UART2(A39C)发送数据
├─ 如果MQTT已连接，通过ESP8266上传数据
└─ 延时1秒后重复循环
```

#### ESP8266 MQTT连接流程

1. AT+RST（复位模块）
2. 等待WiFi自动连接（ESP8266已预先配置好SSID和密码）
3. AT+MQTTUSERCFG（配置MQTT用户参数）
4. AT+MQTTCLIENTID（配置Client ID）
5. AT+MQTTCONN（连接阿里云MQTT服务器）

#### 数据上传格式

```json
{
  "id": "123",
  "version": "1.0",
  "params": {
    "EnvTemperature": 25.5,
    "EnvHumidity": 60.2,
    "GasConcentration": 300
  }
}
```

---

### ProjectB - 接收端

#### 功能概述

- 接收A39C无线数据
- 解析环境参数
- ST7735屏幕实时显示

#### 工作流程

```
初始化
├─ 系统时钟配置
├─ 外设初始化（GPIO、UART、SPI、DMA）
├─ A39C配置
├─ ST7735屏幕初始化
└─ 显示启动界面（5秒）

主循环
├─ 通过UART2接收A39C数据（逐字节接收）
├─ 缓冲数据直到遇到换行符（\r或\n）
├─ 解析数据格式：
│   ├─ 查找"T"标识 → 提取温度
│   ├─ 查找"H"标识 → 提取湿度
│   └─ 查找"S"标识 → 提取烟雾浓度
├─ 刷新屏幕显示：
│   ├─ 显示中文标题"环境参数"
│   ├─ 显示温度（如：温度:25.5℃）
│   ├─ 显示湿度（如：湿度:60.2%）
│   ├─ 显示烟雾浓度（如：烟雾浓度:300ppm）
│   └─ 显示装饰图片
└─ 持续循环等待下一包数据
```

---

## 关键技术实现

### 1. DHT12传感器数据采集

**特点**：
- 单次测量需要约2.5秒稳定时间
- 温度范围：-40℃~+125℃，精度±0.5℃
- 湿度范围：0~100%RH，精度±2%RH

**数据转换公式**：

```c
// 温度
temperature = 400 + temp_raw / 25.6;  // 输出×10

// 湿度
humidity = (hum_raw * 100.0f / 65536.0f) * 10.0f;  // 输出×10
```

### 2. MQ2烟雾浓度采集

**特点**：
- 通过ADC1采集模拟电压
- 12位ADC精度（0-4095）
- 线性化处理为0-1000ppm浓度值

### 3. A39C无线模块配置

**配置引脚**：
- MD0 = PA1 → 高电平
- MD1 = PA4 → 低电平

**通信参数**：
- 波特率：9600bps
- 数据位：8
- 停止位：1
- 校验位：无

### 4. ESP8266 MQTT上云

**阿里云连接参数**：
- MQTT服务器：`iot-06z00hqfmjy48dl.mqtt.iothub.aliyuncs.com`
- 端口：1883
- Topic：`/sys/{ProductKey}/{DeviceName}/thing/event/property/post`
- Client ID：`{ProductKey}.{DeviceName}|securemode=2,signmethod=hmacsha256,timestamp=1776479622131|`

### 5. ST7735屏幕显示

**屏幕规格**：
- 分辨率：128×160
- 颜色深度：16位RGB565
- 驱动芯片：ST7735

**显示内容**：
- 中文字库（自定义点阵）
- 环境参数实时刷新
- 装饰性图片

---

## 数据格式定义

### A39C无线传输格式

```
"T" + 温度值(浮点数) + "H" + 湿度值(浮点数) + "S" + 烟雾浓度(整数) + "\r\n"
```

**示例**：

```
T25.5H60.2S300\r\n
```

### 阿里云MQTT上传格式

见上文JSON格式。

---

## 项目文件结构

```
Keil5NewProject/
└─ Project/
   ├─ ProjectA/              # 发送端
   │  ├─ Core/
   │  │  ├─ Inc/             # 头文件
   │  │  │  ├─ dth12.h
   │  │  │  ├─ esp8266.h
   │  │  │  └─ st7735.h
   │  │  └─ Src/             # 源文件
   │  │     ├─ main.c
   │  │     ├─ dth12.c
   │  │     └─ esp8266.c
   │  └─ MDK-ARM/            # Keil5工程文件
   └─ ProjectB/              # 接收端
      ├─ Core/
      │  ├─ Inc/             # 头文件
      │  └─ Src/             # 源文件
      │     ├─ main.c
      │     └─ st7735.c
      └─ MDK-ARM/            # Keil5工程文件
```

---

## 项目特点与创新

1. **双模块设计**：发送采集+接收显示，数据无线传输
2. **双重备份**：本地显示+云端存储，数据安全可靠
3. **低延迟无线传输**：433MHz频段，穿透力强，适用于室内环境
4. **MQTT标准协议**：阿里云IoT平台，便于后续功能扩展
5. **友好中文界面**：ST7735彩色屏幕，实时数据一目了然
6. **模块化代码**：功能清晰划分，便于维护和扩展

---

## 应用场景

- 家庭环境监测
- 办公室环境监测
- 仓库环境监控
- 实验室环境监控
- 智能家居系统节点

---

## 快速开始

### 环境要求

- Keil MDK-ARM 5.x
- STM32CubeMX（可选，用于重新配置外设）
- ST-Link/J-Link调试器

### 编译与烧录

1. 克隆项目到本地
   ```bash
   git clone https://github.com/your-username/your-repo.git
   ```

2. 使用Keil打开对应工程文件
   - 发送端：`Project/ProjectA/MDK-ARM/ProjectA.uvprojx`
   - 接收端：`Project/ProjectB/MDK-ARM/ProjectB.uvprojx`

3. 编译项目（Rebuild）

4. 连接调试器，下载程序到STM32

### 配置说明

1. **ESP8266 WiFi配置**：首次使用需通过AT指令配置WiFi SSID和密码
2. **阿里云IoT平台**：需要在阿里云控制台创建产品和设备，并更新代码中的ProductKey、DeviceName等信息
3. **A39C无线模块**：确保两个模块配置相同的通信参数

---

## 许可证

MIT License

---

## 联系方式

如有问题或建议，欢迎提交Issue或Pull Request。

