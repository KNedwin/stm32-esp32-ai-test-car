# GPIO 与引脚（导航，统一在 components/common/pins.h）
| 引脚 | 功能 | 证据 |
|---|---|---|
| GPIO1 | 电位器 ADC1_CH0（**已退役**：ADC 无调用方，悬空不再影响） | components/common/pins.h:PIN_ADC_POT |
| GPIO2 | LED1（低电平亮） | components/common/pins.h:PIN_LED1 |
| GPIO4 | 电机 LEDC CH0 | components/common/pins.h:PIN_MOTOR_PWM1 |
| GPIO5 | 电机 LEDC CH1 | components/common/pins.h:PIN_MOTOR_PWM2 |
| GPIO8 | LED2 | components/common/pins.h:PIN_LED2 |
| GPIO9 | LED3 | components/common/pins.h:PIN_LED3 |
| GPIO10 | UART1 TX 读卡 | components/common/pins.h:PIN_RFID_TX |
| GPIO11 | UART1 RX 读卡 | components/common/pins.h:PIN_RFID_RX |
| GPIO12 | UART2 TX TTS | components/common/pins.h:PIN_TTS_TX |
| GPIO13 | UART2 RX TTS | components/common/pins.h:PIN_TTS_RX |
| GPIO48 | 板载 WS2812B RGB LED（RMT TX） | components/common/config.h:LED_WS2812_PIN / ws2812.c |
| USB-DP/DM | USB-Serial-JTAG console | sdkconfig.defaults:CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG |