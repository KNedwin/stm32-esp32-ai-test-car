# 核心数据结构
- `trigger_rule_t`（components/common/config.h）: 触发词规则，TRIGGER_RULES_MAX=8
- `rfid_logic_t` / `motor_logic_t`: 与 STM32 版逐字相同（四版共享）
- `rfid_control_t`（main/rfid_process.h）: wait_tick/wait_resend_times/led_tick/logic
- `CMD`（components/common/card_parse.h）: ReceiveBuffer[32]/block_data[16]
- 时间基准: esp_timer_get_time()/1000（32 位截断，差值比较回绕安全）
