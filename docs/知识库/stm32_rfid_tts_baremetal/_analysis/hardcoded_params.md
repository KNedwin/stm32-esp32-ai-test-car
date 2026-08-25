# 硬编码参数（导航）
## 编译期默认（config.h，行号以当前源码为准）
| 参数 | 值 | 含义 |
|---|---|---|
| MOTOR_START_LATE_TIME_MS | 2000 | A 晚启动 |
| MOTOR_START_SLOW_TIME_MS | 4000 | B 缓启动 |
| MOTOR_SPEED_MAX/TARGET_SPEED | 999 | 速度上限/目标 |
| MOTOR_MAX_RUN_TIME_MS | 1000000 | 1000s 上限 |
| MOTOR_TIME_START_S / DURATION_S | 42 / 5 | E/G 降速窗口 |
| MOTOR_SPEED_PERCENT | 50 | F 降速百分比 |
| TRIGGER_RULES | 太阳(1次)/地球(2次) GBK | 触发词表 |
| TRIGGER_COUNT_INTERVAL_MS | 10000 | 计数间隔 |
| TRIGGER_STOP_RAMP_TIME_S / WAIT_TIME_S | 2 / 10 | H/I 停车序列 |
| LED_ON_TIME_S | 3 | C LED 熄灭延时 |
| SPEAK_DEDUP_TIME_S | 10 | D 去重窗口 |
| RFID_READ_DELAY_MS / LED_POLL_MS | 800 / 10 | 播报后延时/轮询 |
| RFID_READ_TIMEOUT_MS | 20 | 读块超时 |
| RES_MAX, STOP_TIME_MIN/MAX_MS | 5000, 10000/600000 | 电位器公式（已退役仅逻辑层保留） |
| DBG_USART_BAUD 等 | 115200 | 调试口 |
## Flash 存储与运行时范围（config/nvs_params.c，2026-08 新增）
- PARAMS_PAGE_ADDR=0x0800FC00，PARAMS_MAGIC=0xA55A，CRC16-MODBUS(poly 0xA001)
- autostop_ms 默认 300000ms(5min)，钳制 10s~1000s
- sanitize 钳制: late 0.5~20s / slow 0.5~30s / slowwin start≤3600s dur 0.2~120s pct 5~95 / rule len1~16 cnt1~10 / count_interval 1~60s / stop_ramp 0.5~15s / wait 1~120s / led 0.5~30s / dedup 0.5~60s / poll 0.1~5s
- CLI SET 键名（秒单位）: late_s slow_s target dir sr_s ws_s led_s dedup_s poll_s ci_s as_s + win_add/win_del/rule_add/rule_del
