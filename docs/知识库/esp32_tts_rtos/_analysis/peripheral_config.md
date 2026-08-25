# 外设配置（导航）
| 外设 | 配置 | 关键参数 | 证据 |
|---|---|---|---|
| UART1 | 读卡 9600→115200 | 环形缓冲 256B；init 后发 0x2C 切 115200 | components/common/card_uart.c |
| UART2 | TTS 9600 | 8N1；<S>3/<V>6/**<I>7** 默认设置 | components/common/tts.c |
| LEDC | 电机双通道互补 | 20kHz 10-bit；Motor_SetDirection 正/反转交换主从 | components/common/motor_drv.c |
| SARADC1_CH0 | 电位器（**已退役**：仍编译、无调用方） | adc_oneshot 12-bit 衰减 12dB | components/common/adc.c |
| RMT TX | WS2812B GPIO48 | rmt_new_simple_encoder+callback；GRB 字节序；10MHz | components/common/ws2812.c |
| GPIO | LED×3 | 推挽+上拉 初始高 | components/common/led.c |
| WiFi SoftAP | EV-Car-Setup / WPA_WPA2_PSK / 12345678 / ch1 / max4 | IP 192.168.4.1（配网模式） | components/common/wifi_ap.c |
| HTTP Server | :80 GET / 、GET+POST /api/params、POST /api/restart | max_uri_handlers=8 | components/common/web_server.c |
| NVS | "evcar"/"params" blob + "evcar"/"cfg_cnt" u8 | len==sizeof 校验，不符回落默认一次 | components/common/nvs_params.c config_mode.c |
| USB-Serial-JTAG | console 调试 | printf 输出 | sdkconfig.defaults |