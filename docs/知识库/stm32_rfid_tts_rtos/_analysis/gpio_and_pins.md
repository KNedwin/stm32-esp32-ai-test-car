# GPIO 与引脚（导航）
| 引脚 | 功能 | 证据 |
|---|---|---|
| PA0 | TIM2_CH1 电机 PWM | Core/Src/tim.c:HAL_TIM_MspPostInit（AF_PP） |
| PA1 | TIM2_CH2 电机 PWM | Core/Src/tim.c:HAL_TIM_MspPostInit |
| PA2 | USART2_TX TTS | Core/Src/usart.c:HAL_UART_MspInit |
| PA3 | USART2_RX TTS | Core/Src/usart.c:HAL_UART_MspInit |
| PA8 | LED（低电平亮） | Core/Src/gpio.c:MX_GPIO_Init（SET 上拉） |
| PA9 | USART1_TX 读卡 | Core/Src/usart.c:HAL_UART_MspInit |
| PA10 | USART1_RX 读卡 | Core/Src/usart.c:HAL_UART_MspInit |
| PA13/14 | SWD | Core/Src/gpio.c（Serial Wire） |
| PB1 | ADC1_IN9 电位器（已退役：MX_ADC1_Init 仍执行但无采样调用方） | Core/Src/adc.c:HAL_ADC_MspInit |
| PB10 | USART3_TX 调试输出 + CLI 应答 | Core/Src/usart.c:HAL_UART_MspInit |
| PB11 | USART3_RX 参数配置 CLI 接收（ParamCli_Init 置 CR1_RE） | Core/Src/usart.c + config/param_cli.c:26 |
| PB12 | LED | Core/Src/gpio.c:MX_GPIO_Init |
| PC13 | LED | Core/Src/gpio.c:MX_GPIO_Init |
| PD0/1 | HSE 8MHz 晶振 | Core/Src/main.c:RCC |

- AFIO 重映射: 无（全库无 GPIO_PinRemapConfig 调用）
- 无重映射，引脚以 MspInit/MspPostInit 实配为准