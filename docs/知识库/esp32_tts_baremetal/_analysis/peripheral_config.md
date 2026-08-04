# 外设配置（导航）
| 外设 | 配置 | 关键参数 | 证据 |
|---|---|---|---|
| UART1 | 读卡 9600→115200 | 环形缓冲 256B；init 后发 0x2C 切 115200 | components/common/card_uart.c |
| UART2 | TTS 9600 | 8N1；TTS_Send/TTS_SetupDefaults | components/common/tts.c |
| LEDC | 电机双通道互补 | 20kHz 10-bit；speed=0 两路 DUTY_MAX | components/common/motor_drv.c |
| ADC | 电位器 ADC1_CH0 | adc_oneshot，12-bit，衰减 12dB | components/common/adc.c |
| GPIO | LED×3 | 推挽+上拉 初始高 | components/common/led.c |
| USB-Serial-JTAG | console 调试 | printf 输出 | sdkconfig.defaults |
