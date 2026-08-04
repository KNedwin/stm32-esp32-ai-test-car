# 启动顺序与任务
## 启动顺序（app_main）
LED_Init → Motor_Drv_Init → Motor_Control(0) → Dbg_Init → Card_Uart_Init（含波特率切换）→ TTS_Init → ADC_Init
→ xTaskCreate(RFID_Task pri=5, Motor_Task pri=1)
## 任务（RTOS 版）
| 任务 | 优先级 | 栈 | 职责 |
|---|---|---|---|
| RFID_Task | 5 | 4096B | 读卡/播报/LED/触发（rfid_task.c） |
| Motor_Task | 1 | 2048B | 电机状态机（motor_task.c，喂 motor_logic） |
