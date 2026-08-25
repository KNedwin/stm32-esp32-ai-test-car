# 硬编码参数（导航，编译期=config.h+pins.h；运行期=nvs_params.h 默认表）
| 参数 | 值 | 含义 |
|---|---|---|
| MOTOR_START_LATE_TIME_MS | 2000 | A 晚启动（=late_ms 默认） |
| MOTOR_START_SLOW_TIME_MS | 4000 | B 缓启动（=slow_ms 默认） |
| MOTOR_TIME_START_S/DURATION_S/PERCENT | 42/5/50 | 默认减速窗口 slowwins[0] |
| TRIGGER_RULES | 太阳/地球 | 触发词默认表 |
| TRIGGER_COUNT_INTERVAL_MS | 10000 | 计数间隔（=count_interval_ms 默认） |
| TRIGGER_STOP_RAMP_TIME_S | 2 | H 减速（=stop_ramp_ms 默认） |
| TRIGGER_WAIT_TIME_S | 10 | I 静止（=wait_ms 默认） |
| TRIGGER_ACK_GREEN_MS | 500 | 触发绿色确认窗（未参数化） |
| LED_ON_TIME_S | 3 | C 卡离场延时（=led_on_ms 默认） |
| SPEAK_DEDUP_TIME_S | 10 | D 去重窗口（=dedup_ms 默认） |
| RFID_READ_DELAY_MS | 800 | 播报后延时（=rfid_poll_ms 默认） |
| RFID_READ_TIMEOUT_MS | 20 | 读块超时（未参数化） |
| RFID_LED_POLL_MS / NONE 探测周期 | 10ms / 200ms | 未参数化 |
| autostop_ms 默认 | 300000 | 自动停车 5 分钟（sanitize 10s~1000s） |
| RES_MAX/STOP_TIME_MIN/MAX_MS | 5000/10s/600s | 仅 MotorLogic_CalcStopTime 共享层使用 |
| MOTOR_MAX_RUN_TIME_MS | 1000000 | 1000s 绝对上限 |
| DEMO_MODE | 0 | 演示模式开关（代码路径保留） |
| MOTOR_PWM_FREQ_HZ/RES_BITS | 20000/10 | 电机 PWM |
| 引脚 | GPIO1(退役)/2/4/5/8/9/10/11/12/13/48 | pins.h + LED_WS2812_PIN |
| WiFi AP | EV-Car-Setup/WPA2/12345678/ch1/max4 | wifi_ap.c |

