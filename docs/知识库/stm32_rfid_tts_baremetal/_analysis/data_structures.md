# 核心数据结构
- `trigger_rule_t`（config.h）: GBK 触发词规则（word/len/count_req/speak_en），表 TRIGGER_RULES，上限 TRIGGER_RULES_MAX=8
- `rfid_logic_t`（Task/rfid_logic.h）: 触发计数/去重状态（trig_count/trig_last_count_tick/trig_triggered/last_speak）
- `motor_logic_t`（Task/motor_logic.h）: 电机状态机状态（state/start_tick/state_tick/stop_time/speed/ramp_start/pending_trigger）
- `rfid_control_t`（Task/rfid_process.h）: 读卡状态机（chinese_data/block_num/wait_tick/led_tick/logic）
- `CMD`（hardware/rfid_card/Card.h）: 读卡帧缓冲（ReceiveBuffer[32]/block_data[16]）
- `card_res_flag`（CARD_FLAG_*）: 五态读卡标志（NONE/RESDATA/WAIT/EXIST/LEDLIGHT）
