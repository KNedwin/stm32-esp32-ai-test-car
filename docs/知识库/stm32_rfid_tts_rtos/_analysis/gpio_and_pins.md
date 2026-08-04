# GPIO 与引脚（导航）
| 引脚 | 功能 | 证据 |
|---|---|---|
| PA0 | TIM2_CH1 电机 PWM | sources/tim.c:HAL_TIM_MspPostInit（AF_PP） |
| PA1 | TIM2_CH2 电机 PWM | sources/tim.c:HAL_TIM_MspPostInit |
| PA2 | USART2_TX TTS | sources/usart.c:HAL_UART_MspInit |
| PA3 | USART2_RX TTS | sources/usart.c:HAL_UART_MspInit |
| PA8 | LED（低电平亮） | sources/gpio.c:MX_GPIO_Init（SET 上拉） |
| PA9 | USART1_TX 读卡 | sources/usart.c:HAL_UART_MspInit |
| PA10 | USART1_RX 读卡 | sources/usart.c:HAL_UART_MspInit |
| PA13/14 | SWD | sources/gpio.c（Serial Wire） |
| PB1 | ADC1_IN9 电位器 | sources/adc.c:HAL_ADC_MspInit（模拟输入） |
| PB10 | USART3_TX 调试输出 | sources/usart.c:HAL_UART_MspInit |
| PB11 | USART3_RX 调试输出 | sources/usart.c:HAL_UART_MspInit |
| PB12 | LED | sources/gpio.c:MX_GPIO_Init |
| PC13 | LED | sources/gpio.c:MX_GPIO_Init |
| PD0/1 | HSE 8MHz 晶振 | sources/main.c:RCC |
