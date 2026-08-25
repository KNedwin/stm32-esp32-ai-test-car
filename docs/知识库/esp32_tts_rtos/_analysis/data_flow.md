# 数据流（导航）
## 读卡链路
UART1 环形缓冲 → Card_Uart_Poll（每圈轮询）→ Card_Parse_Feed → card_res_flag
→ RFID 状态机 → chinese_data[16] → RfidLogic_Process → 事件
→ EV_SPEAK(_FORCED): TTS_Send / EV_TRIGGER_STOP: s_trig_pending 绿色确认窗 500ms → motor_trigger_flag
## 电机链路（2026-08-25: 无 ADC，stop_time 来自参数层）
g_params.autostop_ms → MotorLogic_Init(stop_time) → MotorLogic_Step(每拍) → Motor_Control(dir) → LEDC PWM
## 触发停车链路
RfidLogic EV_TRIGGER_STOP → 绿窗 500ms → motor_trigger_flag → MotorLogic_Step 消费 → STOPPING→WAIT→RAMPUP
## 参数闭环链路
webpage.html 表单(秒) → POST /api/params JSON → apply_json_to_params → utf8_to_gbk(触发词)
→ params_save(sanitize 钳制+NVS blob) → esp_restart → params_init 加载 → SetTiming/SetConfig/MotorLogic_Init 注入生效