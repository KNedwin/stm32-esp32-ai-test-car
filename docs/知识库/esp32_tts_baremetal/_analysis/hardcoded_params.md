# 硬编码参数（导航，config.h + pins.h + nvs_params.c 默认值）
## 编译期宏（config.h）
| 参数 | 值 | 含义 |
|---|---|---|
| MOTOR_START_LATE_TIME_MS | 2000 | A 晚启动（=参数层默认 late_ms） |
| MOTOR_START_SLOW_TIME_MS | 4000 | B 缓启动 |
| MOTOR_TIME_START_S | 42 | E 降速起点（默认窗口 start_ms=42000） |
| MOTOR_SPEED_PERCENT | 50 | F 降速百分比 |
| MOTOR_TIME_DURATION_S | 5 | G 降速时长 |
| TRIGGER_RULES | 太阳/地球 | 默认触发词表（太阳 1 次/地球 2 次，均播报） |
| TRIGGER_COUNT_INTERVAL_MS | 10000 | 计数间隔 |
| TRIGGER_STOP_RAMP_TIME_S | 2 | H 减速时长 |
| TRIGGER_WAIT_TIME_S | 10 | I 静止等待 |
| TRIGGER_ACK_GREEN_MS | 500 | 触发后绿色确认窗 |
| LED_ON_TIME_S | 3 | C（默认 led_on_ms=3000） |
| SPEAK_DEDUP_TIME_S | 10 | D（默认 dedup_ms=10000） |
| RFID_READ_DELAY_MS | 800 | 播报后延时（默认 rfid_poll_ms=800） |
| RFID_LED_POLL_MS | 10 | LED 保持期轮询间隔 |
| RFID_READ_TIMEOUT_MS | 20 | 读块超时 |
| MOTOR_PWM_FREQ_HZ | 20000 | 电机 PWM 频率 |
| MOTOR_PWM_RES_BITS | 10 | LEDC 分辨率 |
| LED_WS2812_PIN | 48 | 板载 RGB |
| DEMO_MODE | 0 | 演示模拟开关（保留未启用） |
| RES_MAX / STOP_TIME_MIN/MAX_MS | 5000/10s/600s | 电位器量程（仅剩共享层 CalcStopTime 使用） |
| 引脚 | GPIO1(退役)/2/4/5/8/9/10/11/12/13/48 | pins.h |

## 运行时参数默认值（nvs_params.c PARAMS_DEFAULT，网页可改）
late=2000 slow=4000 target=999 dir=0 slowwin[{42000,5000,50}] rules{太阳×1,地球×2} count_interval=10000 stop_ramp=2000 wait=10000 led_on=3000 dedup=10000 rfid_poll=800 **autostop=300000**
（2026-08 更新：新增绿窗/WS2812/DEMO_MODE/参数默认值条目）