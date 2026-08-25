# 外设配置（导航）
| 外设 | 配置 | 关键参数 | 证据 |
|---|---|---|---|
| USART1 | 读卡 9600→115200 | 8N1；init 后 SetBound115200 切 115200；RX 中断 | Core/Src/usart.c；hardware/rfid_card/Card.c |
| USART2 | TTS 9600 | 8N1；__io_putchar 重定向（printf 归 TTS） | Core/Src/usart.c；hardware/USART/BSP_USART.c |
| USART3 | 调试输出 + 参数配置 CLI | 115200 8N1；Dbg_Printf 发送、ParamCli_Poll 轮询接收（CR1_RE 手动使能） | Core/Src/usart.c；hardware/DEBUG/Debug.c；config/param_cli.c |
| TIM2 | 电机 PWM 1kHz | PSC=71 ARR=999 PWM2 双通道 OCFast ENABLE；Motor_SetDirection 差分角色交换 | Core/Src/tim.c；hardware/pwm/PWM.c |
| ADC1 | 电位器 IN9（**已退役**） | 28.5 cycles 连续转换；初始化仍执行，Get_ADC_Value 无调用方 | Core/Src/adc.c；hardware/ADC/BSP_ADC.c |
| GPIO | LED×3 | 推挽+上拉 初始高 | Core/Src/gpio.c |
| FLASH | 参数页 0x0800FC00(1KB) | HAL_FLASHEx_Erase(FLASH_TYPEERASE_PAGES)+Program WORD | config/nvs_params.c:params_save |