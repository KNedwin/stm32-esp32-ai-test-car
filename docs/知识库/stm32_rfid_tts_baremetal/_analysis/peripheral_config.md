# 外设配置（导航）
| 外设 | 配置 | 关键参数 | 证据 |
|---|---|---|---|
| USART1 | 读卡 9600→115200 | 8N1；init 后 SetBound115200 切 115200；NVIC 抢占5 | sources/usart.c:USART1_Init 2；hardware/rfid_card/Card.c |
| USART2 | TTS 9600 | 8N1；__io_putchar 重定向（printf→语音口！） | sources/usart.c；hardware/USART/BSP_USART.c |
| USART3 | 调试输出+串口配置 CLI 115200 | 8N1；Dbg_Printf 输出；CubeMX 只配 TX，ParamCli_Init 补开 RX(RE 位)；轮询收包非中断 | sources/usart.c；hardware/DEBUG/Debug.c；config/param_cli.c |
| TIM2 | 电机 PWM 1kHz | PSC=71 ARR=999 PWM2 双通道 OCFast ENABLE（CH1/CH2 一致） | sources/tim.c:MX_TIM2_Init |
| ADC1 | 电位器 IN9 28.5cycles——**已退役**仍编译 | 无调用方；MotorLogic_CalcStopTime 逻辑层保留未用 | sources/adc.c（grep 无 Get_ADC_Value 调用） |
| GPIO | LED×3 推挽+上拉 初始高 | PC13/PB12/PA8 | sources/gpio.c |
| 内部 Flash | 参数存储末页 0x0800FC00(1KB) | HAL_FLASHEx_Erase FLASH_TYPEERASE_PAGES + WORD 编程 | config/nvs_params.c:params_save |
