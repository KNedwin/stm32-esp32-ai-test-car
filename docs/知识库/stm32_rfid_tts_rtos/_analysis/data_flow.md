# 数据流（导航）
## 读卡链路
USART1 RX 中断 → HAL_UART_RxCpltCallback（Card.c）→ 帧解析 UartReceiveCommand → card_res_flag
→ RFID 状态机（EXIST→WAIT→RESDATA→LEDLIGHT→NONE）→ chinese_data[16]
→ RfidLogic_Process（规则表来自 RfidLogic_SetConfig 注入）→ 事件 → TTS 播报（USART2）/ motor_trigger_flag

## 电机链路（电位器退役后）
Motor_Init: Motor_SetDirection(g_params.motor_dir) + MotorLogic_Init(stop_time=g_params.autostop_ms 默认300000ms)
→ 每拍 MotorLogic_Step（时序来自 MotorLogic_SetTiming 注入）→ Motor_Control → TIM2 PWM 差分输出

## 触发停车链路
RfidLogic 返回 EV_TRIGGER_STOP → motor_trigger_flag=1 → MotorLogic_Step 消费 → STOPPING(g_params.stop_ramp_ms)→WAIT(g_params.wait_ms)→RAMPUP

## 参数配置流（新增）
USART3 收行（defaultTask 轮询）→ param_cli_execute 解析 → 改 g_params 字段 → params_apply()（MotorLogic_SetTiming + RfidLogic_SetConfig 即时注入）
→ 可选 SAVE: params_save() 擦写 Flash 末页（magic 0xA55A+CRC16MODBUS）；上电 params_init 读回校验失败回落 config.h 默认