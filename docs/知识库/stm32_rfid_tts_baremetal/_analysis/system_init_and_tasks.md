# 启动顺序与任务
## 启动顺序（main，2026-08 版）
HAL_Init → SystemClock_Config(72MHz) → MX_GPIO/ADC1/TIM2/USART1/USART2/USART3_Init
→ LED_Sta(0) → PWM_Init() → Motor_Control(0) → Dbg_Init()
→ **params_init()**（读 Flash 末页参数+CRC，失败回落默认；sources/config/nvs_params.c:150）
→ **params_apply()**（g_params 注入 MotorLogic_SetTiming/RfidLogic_SetConfig；nvs_params.c:35）
→ **ParamCli_Init()**（使能 USART3 接收 RE 位；config/param_cli.c:22）
→ Motor_Init()（转向参数+MotorLogic_Init(stop_time=g_params.autostop_ms)；motor_process.c:13）
→ RFID_Init()（TTS <S>3/<V>6/<I>7 提示音7号 + 读卡参数；rfid_process.c:28）
→ while(1){ RFID_Process; Motor_Process; ParamCli_Poll; if(ParamCli_ShouldEnterIsp()) HAL_Delay(100)+ISP_Enter }
（sources/Core/Src/main.c USER CODE 2 区与 WHILE 区）
- USART1 init 内完成波特率切换（先使能接收防 ORE → SetBound115200 → 重 init 115200）
## 主循环"任务"（裸机协作式，每圈轮询）
| 循环体调用 | 周期 | 文件 |
|---|---|---|
| RFID_Process | 每圈（五态非阻塞状态机） | Task/rfid_process.c:49 |
| Motor_Process | 每圈（电机绝对计时状态机） | Task/motor_process.c:30 |
| ParamCli_Poll | 每圈（USART3 行协议轮询，RXNE 逐字节） | config/param_cli.c:211 |
| ISP 处理 | 命令请求时软跳 Bootloader | main.c + config/isp_jump.c:10 |
## 中断
| ISR | 触发源 | 回调链 |
|---|---|---|
| SysTick_Handler | 1kHz 时基 | HAL_IncTick（唯一时基，裸机版 TIM1 不用） |
| USART1_IRQHandler | 读卡模块 RX | HAL_UART_RxCpltCallback → UartReceiveCommand → card_res_flag/rfid_last_card_tick |
| USART2_IRQHandler | TTS RX（未用接收） | HAL_UART_IRQHandler 空转 |
