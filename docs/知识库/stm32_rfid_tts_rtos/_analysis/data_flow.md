# 数据流（导航）
## 读卡链路
USART1 RX 中断 → HAL_UART_RxCpltCallback（Card.c）→ 帧解析 UartReceiveCommand → card_res_flag
→ RFID 状态机（EXIST→WAIT→RESDATA→LEDLIGHT→NONE）→ chinese_data[16]
→ RfidLogic_Process（触发匹配/计数/去重）→ 事件 → TTS 播报（USART2）/ motor_trigger_flag
## 电机链路
电位器 ADC 采样 → MotorLogic_CalcStopTime → MotorLogic_Step（绝对计时状态机）→ Motor_Control → TIM2 PWM
## 触发停车链路
RfidLogic 返回 EV_TRIGGER_STOP → motor_trigger_flag=1 → MotorLogic_Step 消费 → STOPPING→WAIT→RAMPUP
