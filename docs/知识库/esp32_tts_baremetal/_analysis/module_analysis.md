# 模块分析（导航）
| 模块 | 文件 | 职责 | 核心函数 |
|---|---|---|---|
| 读卡驱动 | components/common/card_uart.c|h | UART1 驱动+命令组帧+波特率切换 | Card_Uart_Init/ReadCard/ReadBlock/Poll |
| 帧解析 | components/common/card_parse.c|h | U13T 帧解析纯逻辑 | UartReceiveCommand/Card_Parse_Feed |
| 触发/去重逻辑 | components/common/rfid_logic.c|h | 触发词匹配/计数/去重（四版共享） | RfidLogic_Process/TriggerMatch |
| 电机状态机 | components/common/motor_logic.c|h | 电机时序纯逻辑（四版共享） | MotorLogic_Step/CalcStopTime |
| 电机驱动 | components/common/motor_drv.c|h | LEDC 双通道互补 | Motor_Control/Motor_Drv_Init |
| TTS | components/common/tts.c|h | 发送+默认设置 | TTS_Send/SetupDefaults/Init |
| LED/ADC/DEBUG | components/common/led.c adc.c debug.c | 灯控/电位器/console 输出 | LED_Sta/Get_ADC_Value/Dbg_Printf |
| 读卡应用 | main/rfid_process.c|h | 五态状态机+播报 | RFID_Task/RFID_Process |
| 电机应用 | main/motor_process.c|h | 采样+喂 motor_logic | Motor_Task/Motor_Process |
| 入口 | main/app_main.c | 初始化+调度 | app_main |