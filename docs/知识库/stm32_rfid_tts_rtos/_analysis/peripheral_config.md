# 外设配置（导航）
| 外设 | 配置 | 关键参数 | 证据 |
|---|---|---|---|
| USART1 | 读卡 9600→115200 | 8N1；init 后 SetBound115200 切 115200 | sources/usart.c:USART1_Init 2；hardware/rfid_card/Card.c |
| USART2 | TTS 9600 | 8N1；__io_putchar 重定向 | sources/usart.c；hardware/USART/BSP_USART.c |
| USART3 | 调试输出 115200 | 8N1；Dbg_Printf | sources/usart.c；hardware/DEBUG/Debug.c |
| TIM2 | 电机 PWM 1kHz | PSC=71 ARR=999 PWM2 双通道 OCFast ENABLE | sources/tim.c:MX_TIM2_Init |
| ADC1 | 电位器 IN9 | 28.5 cycles 连续转换 | sources/adc.c:MX_ADC1_Init |
| GPIO | LED×3 | 推挽+上拉 初始高 | sources/gpio.c |
