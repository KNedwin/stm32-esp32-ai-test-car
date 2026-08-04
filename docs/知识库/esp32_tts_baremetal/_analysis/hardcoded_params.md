# 硬编码参数（导航，全部见 components/common/config.h + pins.h）
| 参数 | 值 | 含义 |
|---|---|---|
| MOTOR_START_LATE_TIME_MS | 2000 | A 晚启动 |
| MOTOR_START_SLOW_TIME_MS | 4000 | B 缓启动 |
| MOTOR_TIME_START_S | 42 | E 降速起点 |
| MOTOR_SPEED_PERCENT | 50 | F 降速百分比 |
| MOTOR_TIME_DURATION_S | 5 | G 降速时长 |
| TRIGGER_RULES | 太阳/地球 | 触发词表 |
| TRIGGER_STOP_RAMP_TIME_S | 2 | H 减速时长 |
| TRIGGER_WAIT_TIME_S | 10 | I 静止等待 |
| LED_ON_TIME_S | 3 | C |
| SPEAK_DEDUP_TIME_S | 10 | D |
| RFID_READ_TIMEOUT_MS | 20 | 读块超时 |
| MOTOR_PWM_FREQ_HZ | 20000 | 电机 PWM 频率 |
| MOTOR_PWM_RES_BITS | 10 | LEDC 分辨率 |
| RES_MAX/STOP_TIME_MIN/MAX | 5000/10s/600s | 电位器 |
| 引脚 | GPIO1/2/4/5/8/9/10/11/12/13 | pins.h |
