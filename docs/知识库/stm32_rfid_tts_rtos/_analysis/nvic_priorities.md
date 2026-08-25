# NVIC 优先级（组 4：4 位抢占）
| 中断 | 抢占优先级 | 说明 |
|---|---|---|
| USART1 | 5 | 读卡接收 |
| USART2 | 5 | TTS（RX 未用） |
| TIM1_UP | 15 | HAL 时基（RTOS 版） |

- USART3 无中断：param_cli 采用任务轮询（RXNE 查询，defaultTask 10ms 节拍）
- SysTick: FreeRTOS 调度节拍