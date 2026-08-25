# 启动顺序与任务
## 启动顺序（Core/Src/main.c → Core/Src/freertos.c）
HAL_Init → SystemClock_Config → MX_GPIO/ADC1/TIM2/USART1/USART2/USART3
→ USER CODE 2 区: LED_Sta(0)/PWM_Init/Motor_Control(0)/Dbg_Init
→ MX_FREERTOS_Init **USER CODE Init 区: params_init()→params_apply()→ParamCli_Init()（必须先于任务创建，g_params 就绪后任务才能用默认值起跑）**
→ 任务创建（defaultTask/RFID_TASK/MOTOR_CONTROL）→ osKernelStart

## 任务（RTOS 版，freertos.c:87-125）
| 任务 | 优先级 | 栈 | 职责 |
|---|---|---|---|
| RFID_TASK | High | 1024 字 | 读卡五态状态机/播报/LED/触发（Task/rfid_task.c；上电 <S>3/<V>6/<I>7 设置） |
| defaultTask | Normal | 512 字（128→512 扩容） | ParamCli_Poll() 10ms 节拍轮询 + ISP 命令处理（freertos.c:134-149） |
| MOTOR_CONTROL | Idle | 512 字 | Motor_Init(g_params 注入)+每拍喂 motor_logic+PWM 输出（motor_control_task.c） |

- 任务间通信: card_res_flag(ISR 置位)、motor_trigger_flag(RFID 置位)、g_params(CLI 写/任务读)