# NVIC 优先级（组 4：4 位抢占）
| 中断 | 抢占优先级 | 说明 |
|---|---|---|
| USART1 | 5 | 读卡接收 |
| USART2 | 5 | TTS（RX 未用） |
| SysTick | 15(默认) | HAL 时基 |
- 注：USART3 收包走 ParamCli_Poll 主循环轮询（RXNE 查询），不占中断。
