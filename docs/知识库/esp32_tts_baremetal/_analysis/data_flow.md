# 数据流（导航）
## 读卡链路
UART1 环形缓冲 → Card_Uart_Poll（每圈轮询）→ Card_Parse_Feed → card_res_flag
→ RFID 状态机 → chinese_data[16] → RfidLogic_Process → 事件 → TTS_Send / s_trig_pending(绿窗 500ms) → motor_trigger_flag

## 电机链路（2026-08：电位器采样已删除）
NVS g_params.autostop_ms/target_speed → MotorLogic_Init → MotorLogic_Step（trigger 挂起消费）→ Motor_Control(speed, dir) → LEDC PWM
（MotorLogic_CalcStopTime 仅存于共享层供 STM32 使用，ESP32 无调用方）

## 触发停车链路
RfidLogic EV_TRIGGER_STOP → s_trig_pending（绿色确认窗 TRIGGER_ACK_GREEN_MS=500ms）→ motor_trigger_flag → MotorLogic_Step 消费 → STOPPING→WAIT→RAMPUP

## 参数配置链路（新增）
webpage.html 表单 → POST /api/params(JSON *_s 秒字段) → web_server apply_json_to_params（秒→ms；rules.text UTF-8→GBK）→ params_save(params_sanitize 钳制 + NVS blob) → esp_restart → params_init 读回全部生效