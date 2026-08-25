# 外设配置（导航）
| 外设 | 配置 | 关键参数 | 证据 |
|---|---|---|---|
| UART1 | 读卡 9600→115200 | 环形缓冲 256B；init 后发 0x2C 切 115200 | components/common/card_uart.c |
| UART2 | TTS 9600 | 8N1；TTS_SetupDefaults 发 <S>3/<V>6/<I>7 | components/common/tts.c |
| LEDC | 电机双通道互补 | 20kHz 10-bit；speed=0 两路 DUTY_MAX 同电位停；方向 s_motor_dir 交换通道角色 | components/common/motor_drv.c |
| RMT TX | WS2812 RGB | GPIO48，10MHz 时基，simple encoder callback，GRB 字节序 | components/common/ws2812.c |
| GPIO | LED×3 | 推挽+上拉，初始高（灭） | components/common/led.c |
| SARADC1 | 电位器 ADC1_CH0 —— **驱动保留但无任何调用方**（电位器退役） | adc_oneshot 12-bit 12dB | components/common/adc.c |
| NVS | 参数 blob(evcar/params) + 配网计数(evcar/cfg_cnt) | nvs_flash_init 于 params_init 内完成 | components/common/nvs_params.c、config_mode.c |
| WiFi SoftAP | EV-Car-Setup | WPA_WPA2_PSK，密码 12345678，信道 1，最大 4 连接，IP 192.168.4.1 | components/common/wifi_ap.c |
| HTTP Server | 端口 80 | GET /（内嵌 webpage.html）、GET/POST /api/params、POST /api/restart | components/common/web_server.c |
| USB-Serial-JTAG | console 调试 | printf 输出（UTF-8） | sdkconfig.defaults |
（2026-08 更新：新增 RMT/NVS/WiFi/HTTP 四外设；ADC 标注退役；TTS 指令更新为 <I>7）