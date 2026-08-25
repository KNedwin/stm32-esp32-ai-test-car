# 核心数据结构
- trigger_rule_t（components/common/config.h）: 编译期默认触发词规则，TRIGGER_RULES_MAX=8
- param_rule_t / param_slowwin_t / params_t（nvs_params.h）: 运行时参数 blob（含 autostop_ms/motor_dir/slowwins[8]/rules[8]/led_on_ms/dedup_ms/rfid_poll_ms/count_interval_ms）
- motor_timing_t（motor_logic.h）: Setter 注入时序 late/slow/stop_ramp/wait + slowwin[8]
- rfid_rule_rt_t（rfid_logic.h）: 运行时触发词条（word[16]/len/count_req/speak_en）
- rfid_logic_t / motor_logic_t: 四版共享逐字相同（motor_logic_t 含 pending_trigger/tm 指针）
- rfid_control_t（main/rfid_process.h）: wait_tick/wait_resend_times/led_tick/poll_tick/logic
- CMD（components/common/card_parse.h）: ReceiveBuffer[32]/block_data[16]
- led_color_t（ws2812.h）: 7 色枚举（BOOT/IDLE/CARD/SLOWING/STOPPED/RAMPUP/OFF）
- 时间基准: esp_timer_get_time()/1000（32 位截断，差值比较回绕安全）