# GPIO 与引脚（导航）
| 引脚 | 功能 | 证据 |
|---|---|---|
| PA0 | TIM2_CH1 电机 PWM（dir=0 正转时拉低） | sources/tim.c:HAL_TIM_MspPostInit；hardware/pwm/PWM.c:Motor_Control |
| PA1 | TIM2_CH2 电机 PWM（dir=1 反转角色交换） | 同上 + PWM.c:Motor_SetDirection |
| PA2 | USART2_TX TTS 9600 | sources/usart.c:HAL_UART_MspInit |
| PA3 | USART2_RX TTS | sources/usart.c:HAL_UART_MspInit |
| PA8 | LED（低电平亮） | main.h:LED2_Pin + gpio.c:MX_GPIO_Init |
| PA9 | USART1_TX 读卡 9600→115200 | sources/usart.c:HAL_UART_MspInit |
| PA10 | USART1_RX 读卡（中断接收） | 同上 |
| PA13/14 | SWD | sources/gpio.c（Serial Wire） |
| PB1 | ADC1_IN9 电位器——**硬件退役**：Get_ADC_Value/BSP_ADC 已无调用方，仅 CubeMX 配置保留编译 | sources/adc.c（无调用方，grep 验证） |
| PB10 | USART3_TX 调试输出 + CLI 应答 | sources/usart.c:HAL_UART_MspInit |
| PB11 | USART3_RX **串口配置 CLI 接收**（ParamCli_Init 置 CR1.RE） | usart.c + config/param_cli.c:22,217 |
| PB12 | LED（低电平亮） | main.h:LED3_Pin + gpio.c |
| PC13 | LED（低电平亮） | main.h:LED1_Pin + gpio.c |
| PD0/1 | HSE 8MHz 晶振 | sources/main.c:RCC |
