# 数据流（导航）
## 读卡链路
UART1 环形缓冲 → Card_Uart_Poll（每圈轮询）→ Card_Parse_Feed → card_res_flag
→ RFID 状态机 → chinese_data[16] → RfidLogic_Process → 事件 → TTS_Send / motor_trigger_flag
## 电机链路
ADC 采样 → MotorLogic_CalcStopTime → MotorLogic_Step → Motor_Control → LEDC PWM
## 触发停车链路
RfidLogic EV_TRIGGER_STOP → motor_trigger_flag → MotorLogic_Step 消费 → STOPPING→WAIT→RAMPUP
