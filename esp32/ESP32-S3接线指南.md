# ESP32-S3 新能源小车接线指南

## 主控板信息

- **芯片型号**：ESP32-S3（Xtensa 双核，512KB SRAM）
- **固件版本**：ESP-IDF v6.0.2

---

## 接线总表

| 功能 | ESP32-S3 引脚 | 外设 | 参数 | 连接对象 |
|------|---------------|------|------|----------|
| 读卡模块 U13T TX | GPIO10 | UART1 | 9600→115200 | U13T 的 RX |
| 读卡模块 U13T RX | GPIO11 | UART1 | | U13T 的 TX |
| 语音模块 CN-TTS TX | GPIO12 | UART2 | 9600 8N1 | CN-TTS 的 RX |
| 语音模块 CN-TTS RX | GPIO13 | UART2 | | CN-TTS 的 TX |
| 电机 PWM 输出 1 | GPIO4 | LEDC 通道0 | 20kHz 10-bit | 电机驱动模块 |
| 电机 PWM 输出 2 | GPIO5 | LEDC 通道1 | 20kHz 10-bit | 电机驱动模块 |
| 电位器 | GPIO1 | SARADC1_CH0 | 12-bit | 电位器中间引脚 |
| LED1 | GPIO2 | GPIO | 低电平亮 | LED（经限流电阻→GND） |
| LED2 | GPIO8 | GPIO | 低电平亮 | LED（经限流电阻→GND） |
| LED3 | GPIO9 | GPIO | 低电平亮 | LED（经限流电阻→GND） |
| 调试输出 | 原生 USB 口 | USB-Serial-JTAG | 115200 | 电脑 USB 口 |

---

## 详细接线说明

### 1. 读卡模块 U13T（RFID）

```
ESP32-S3          U13T 模块
─────────────────────────────
GPIO10 (TX)  ──→  RX
GPIO11 (RX)  ←──  TX
3.3V 或 5V   ──→  VCC
GND          ──→  GND
```

**注意**：
- 串口连接为**交叉连接**（TX→RX，RX→TX）
- U13T 供电范围 3.0~5.5V，**3.3V 即可工作**
- 通信协议：初始波特率 9600，发送特定命令后切换到 115200

### 2. 语音模块 CN-TTS

```
ESP32-S3          CN-TTS 模块
─────────────────────────────
GPIO12 (TX)  ──→  RX
GPIO13 (RX)  ←──  TX
5V           ──→  VCC（必须 5V！）
GND          ──→  GND
```

**注意**：
- **CN-TTS 模块必须 5V 供电**（4.5~5.5V）
- 播报时峰值电流 320mA，需独立供电或稳压模块
- GPIO12/13 均为 5V 容忍（FT）引脚，可直接连接

### 3. 电机驱动模块

```
ESP32-S3          电机驱动模块
─────────────────────────────
GPIO4    ──→  PWM1 输入
GPIO5    ──→  PWM2 输入
3.3V/5V  ──→  逻辑供电
GND      ──→  GND（必须共地）
```

**注意**：
- PWM 参数：20kHz 频率，10-bit 分辨率（0~1023）
- 电机驱动模块需单独供电（根据电机电压选择）
- **务必共地**：ESP32-S3 的 GND 与电机驱动模块的 GND 必须连接

### 4. 电位器

```
ESP32-S3          电位器
─────────────────────────────
GPIO1      ──→  中间引脚（滑动端）
3.3V       ──→  一端
GND        ──→  另一端
```

**注意**：
- 电位器阻值量程：5kΩ（默认配置）
- 用于调节自动停车时间（10秒~600秒线性）

### 5. LED 指示灯

```
ESP32-S3          LED 电路
─────────────────────────────
GPIO2  ──→  限流电阻(220Ω~1kΩ) ──→  LED+  ──→  LED- ──→  GND
GPIO8  ──→  同上
GPIO9  ──→  同上
```

**注意**：
- 低电平点亮（GPIO 输出 0 时 LED 亮）
- 三路 LED 可并联使用，程序同时驱动
- 建议限流电阻 330Ω

### 6. 调试输出（USB）

```
ESP32-S3          电脑
─────────────────────────────
原生 USB 口 ──→  USB 口
```

**注意**：
- 使用 ESP32-S3 的 **USB-Serial-JTAG** 口
- 无需 USB 转 TTL 模块
- 烧录和调试共用同一个 USB 口

---

## 重要注意事项

### 电源要求

1. **CN-TTS 语音模块**：**必须 5V 供电**，峰值电流 320mA
2. **U13T 读卡模块**：3.0~5.5V 均可，3.3V 足够
3. **电机驱动模块**：根据电机电压单独供电
4. **ESP32-S3**：通过 USB 口供电（5V），板载 LDO 降压到 3.3V

### 共地要求

- **所有模块的 GND 必须连接在一起**
- 特别是电机驱动模块与 ESP32-S3 必须共地

### 电平兼容

- ESP32-S3 的 GPIO10/11/12/13 均为 **5V 容忍（FT）引脚**
- 可直接连接 5V 设备的 TX 输出，无需电平转换

---

## 引脚速查表

| GPIO | 功能 | 说明 |
|------|------|------|
| GPIO1 | 电位器 | SARADC1_CH0 |
| GPIO2 | LED1 | 低电平亮 |
| GPIO4 | 电机 PWM1 | LEDC 通道0 |
| GPIO5 | 电机 PWM2 | LEDC 通道1 |
| GPIO8 | LED2 | 低电平亮 |
| GPIO9 | LED3 | 低电平亮 |
| GPIO10 | 读卡 UART1 TX | 5V 容忍 |
| GPIO11 | 读卡 UART1 RX | 5V 容忍 |
| GPIO12 | TTS UART2 TX | 5V 容忍 |
| GPIO13 | TTS UART2 RX | 5V 容忍 |
| USB | 调试输出 | USB-Serial-JTAG |

---

## 编译与烧录

### 环境激活（每次编译前必须执行）

```bash
source <HOME>/.espressif/tools/activate_idf_v6.0.2.sh
```

### 编译命令

```bash
cd esp32/esp32_tts_rtos    # RTOS 版
# 或
cd esp32/esp32_tts_baremetal  # 裸机版

idf.py set-target esp32s3   # 首次执行
idf.py build                # 编译
```

### 烧录与监视

```bash
idf.py -p /dev/ttyACM0 flash      # 烧录
idf.py -p /dev/ttyACM0 monitor    # 串口监视
```

---

## 常见问题

### Q1: 为什么 GPIO10/11/12/13 可以直接连 5V 设备？
A: 这些引脚是 ESP32-S3 的 **5V 容忍（FT）引脚**，可以承受最高 5.5V 的输入电压。

### Q2: CN-TTS 可以用 3.3V 供电吗？
A: **不推荐**。CN-TTS 模块设计要求 4.5~5.5V 供电，3.3V 可能导致播报声音小或无法工作。

### Q3: 调试信息在哪里查看？
A: 使用 `idf.py monitor` 命令，通过 USB 口查看串口输出。

### Q4: 如何修改引脚配置？
A: 修改 `esp32/components/common/pins.h` 文件，重新编译即可。