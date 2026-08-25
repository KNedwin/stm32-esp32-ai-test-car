# GPIO 与引脚（导航，统一在 components/common/pins.h + config.h）
| 引脚 | 功能 | 证据 |
|---|---|---|
| GPIO1 | 电位器 ADC1_CH0 —— **已退役**：motor_process/app_main 不再调用 ADC_Init/Get_ADC_Value，引脚悬空不再影响运行 | components/common/pins.h PIN_ADC_POT |
| GPIO2 | LED1（低电平亮） | components/common/pins.h PIN_LED1 |
| GPIO4 | 电机 LEDC CH0 | components/common/pins.h PIN_MOTOR_PWM1 |
| GPIO5 | 电机 LEDC CH1 | components/common/pins.h PIN_MOTOR_PWM2 |
| GPIO8 | LED2 | components/common/pins.h PIN_LED2 |
| GPIO9 | LED3 | components/common/pins.h PIN_LED3 |
| GPIO10 | UART1 TX 读卡 | components/common/pins.h PIN_RFID_TX |
| GPIO11 | UART1 RX 读卡 | components/common/pins.h PIN_RFID_RX |
| GPIO12 | UART2 TX TTS | components/common/pins.h PIN_TTS_TX |
| GPIO13 | UART2 RX TTS | components/common/pins.h PIN_TTS_RX |
| GPIO48 | 板载 WS2812 RGB LED（RMT TX 驱动，GRB 序） | components/common/config.h LED_WS2812_PIN |
| USB-DP/DM | USB-Serial-JTAG console | sdkconfig.defaults CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG |

## 外设实例映射（pins.h）
- RFID_UART=UART_NUM_1、TTS_UART=UART_NUM_2、ADC_UNIT_1/CH0（退役）、LEDC TIMER_0 + CH0/CH1 低速模式
（2026-08 更新：GPIO1 退役标注、GPIO48 新增）