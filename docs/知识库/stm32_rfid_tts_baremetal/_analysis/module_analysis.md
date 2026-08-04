# 模块分析（导航）
| 模块 | 文件 | 职责 | 核心函数 |
|---|---|---|---|
| 读卡协议 | hardware/rfid_card/Card.c|h | U13T 帧收发/解析、波特率切换、接收回调 | SetBound115200/ReadCard/ReadBlock/UartReceiveCommand/HAL_UART_RxCpltCallback/HAL_UART_ErrorCallback |
| 触发/去重逻辑 | Task/rfid_logic.c|h | 触发词匹配/计数/去重决策（纯逻辑，四版共享） | RfidLogic_Process/TriggerMatch/IsDup/UpdateSpeak |
| 电机状态机 | Task/motor_logic.c|h | 电机时序纯逻辑（四版共享） | MotorLogic_Step/Init/CalcStopTime/IsInStopSequence |
| 读卡状态机 | Task/{'rfid_task.c' if is_rtos else 'rfid_process.c'}|h | 五态读卡流程+播报+LED | RFID_Task/RFID_Process |
| 电机应用 | Task/{'motor_control_task.c' if is_rtos else 'motor_process.c'}|h | 采样+喂 motor_logic+输出 PWM | Motor_Control_Task/Motor_Process |
| TTS 发送 | hardware/USART/BSP_USART.c|h | printf 重定向(__io_putchar→USART2)、字符串发送 | Usartx_SendString |
| 电机驱动 | hardware/pwm/PWM.c|h | Motor_Control(0~999) 双路 PWM2 | Motor_Control/PWM_Init |
| LED | hardware/LED/led.c|h | 三引脚灯控（边沿调试输出） | LED_Sta |
| 电位器 | hardware/ADC/BSP_ADC.c|h | ADC 采样 | Get_ADC_Value |
| 调试输出 | hardware/DEBUG/Debug.c|h | USART3 数据口 | Dbg_Init/Dbg_Printf |