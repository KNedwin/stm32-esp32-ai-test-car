# 核心数据结构
- `params_t`（components/common/nvs_params.h）: 运行时参数总表——late_ms/slow_ms/target_speed/motor_dir/slowwins[8]/rules[8]/count_interval_ms/stop_ramp_ms/wait_ms/led_on_ms/dedup_ms/rfid_poll_ms/**autostop_ms**；NVS blob 整块存储，len==sizeof 不符回落默认一次
- `trigger_rule_t`（components/common/config.h）: 编译期触发词规则（TRIGGER_RULES 默认表源）
- `rfid_logic_t` / `motor_logic_t`: 与 STM32 版逐字相同（四版共享）
- `rfid_control_t`（main/rfid_task.h）: wait_tick/wait_resend_times/led_tick/logic
- `CMD`（components/common/card_parse.h）: ReceiveBuffer[32]/block_data[16]
- 时间基准: esp_timer_get_time()/1000（32 位截断，差值比较回绕安全）