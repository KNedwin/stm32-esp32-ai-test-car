# 启动顺序与任务
## 启动顺序（main）
HAL_Init → SystemClock_Config → MX_GPIO/ADC1/TIM2/USART1/USART2/USART3 → LED_Sta(0)/PWM_Init/Motor_Control(0)/Dbg_Init → MX_FREERTOS_Init + osKernelStart
- USART1 init 内完成波特率切换（先使能接收防 ORE → SetBound115200 → 重 init 115200）
## 任务（RTOS 版）
| 任务 | 优先级 | 栈 | 职责 |
|---|---|---|---|
| RFID_Task | High(2) | 1024 字 | 读卡/播报/LED/触发（rfid_task.c） |
| Motor_Control_Task | Idle(-3) | 512 字 | 电机状态机（motor_control_task.c，喂 motor_logic） |
| defaultTask | Normal | 128 字 | 空转（CubeMX 遗留） |
