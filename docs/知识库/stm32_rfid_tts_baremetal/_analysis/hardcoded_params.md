# 硬编码参数（导航，全部见 config.h，行号以 v0.5 为准）
| 参数 | 值 | 含义 |
|---|---|---|
| MOTOR_START_LATE_TIME_MS | 2000 | A 晚启动 |
| MOTOR_START_SLOW_TIME_MS | 4000 | B 缓启动 |
| MOTOR_SPEED_MAX | 999 | 速度上限 |
| MOTOR_MAX_RUN_TIME_MS | 1000000 | 1000s 上限 |
| MOTOR_TIME_START_S | 42 | E 降速起点 |
| MOTOR_SPEED_PERCENT | 50 | F 降速百分比 |
| MOTOR_TIME_DURATION_S | 5 | G 降速时长 |
| TRIGGER_RULES | 太阳/地球 GBK | 触发词表 |
| TRIGGER_COUNT_INTERVAL_MS | 10000 | 计数间隔 |
| TRIGGER_STOP_RAMP_TIME_S | 2 | H 减速时长 |
| TRIGGER_WAIT_TIME_S | 10 | I 静止等待 |
| LED_ON_TIME_S | 3 | C LED 熄灭延时 |
| SPEAK_DEDUP_TIME_S | 10 | D 去重窗口 |
| RFID_READ_DELAY_MS | 800 | 播报后延时 |
| RFID_LED_POLL_MS | 10 | LED 轮询 |
| RFID_READ_TIMEOUT_MS | 20 | 读块超时 |
| RES_MAX | 5000 | 电位器量程 |
| STOP_TIME_MIN_MS/MAX_MS | 10000/600000 | 停车时间范围 |
| DBG_ECHO_* | 1 | 调试输出开关 |
