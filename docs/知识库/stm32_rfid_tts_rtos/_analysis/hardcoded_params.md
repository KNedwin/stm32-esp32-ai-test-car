# 硬编码参数（导航）
## 编译期默认（config.h，运行时被 g_params 覆盖为初值来源）
| 参数 | 值 | 含义 |
|---|---|---|
| MOTOR_START_LATE_TIME_MS | 2000 | A 晚启动（=g_params.late_ms 默认） |
| MOTOR_START_SLOW_TIME_MS | 4000 | B 缓启动 |
| MOTOR_SPEED_MAX / MOTOR_TARGET_SPEED | 999 | 速度上限/目标 |
| MOTOR_MAX_RUN_TIME_MS | 1000000 | 1000s 绝对上限（仍硬编码于 motor_logic.c:98） |
| MOTOR_TIME_START_S / DURATION_S / SPEED_PERCENT | 42/5/50 | E/G/F 默认降速窗口 |
| TRIGGER_RULES | 太阳(1次)/地球(2次) GBK | 触发词默认表 |
| TRIGGER_COUNT_INTERVAL_MS | 10000 | 计数间隔（=count_interval_ms 默认） |
| TRIGGER_STOP_RAMP_TIME_S / WAIT_TIME_S | 2/10 | H/I 默认 |
| LED_ON_TIME_S | 3 | C（=led_on_ms 默认） |
| SPEAK_DEDUP_TIME_S | 10 | D（=dedup_ms 默认） |
| RFID_READ_DELAY_MS | 800 | **=rfid_poll_ms 默认**（LED 保持期轮询间隔，注意非 RFID_LED_POLL_MS） |
| RFID_LED_POLL_MS | 10 | 已无使用方（宏残留） |
| RFID_READ_TIMEOUT_MS | 20 | 读块超时重发节拍 |
| RES_MAX / STOP_TIME_MIN_MS / STOP_TIME_MAX_MS | 5000/10000/600000 | MotorLogic_CalcStopTime 插值保留（无调用方） |
| DBG_USART_BAUD / DBG_ECHO_* | 115200/1 | 调试口 |

## 运行时参数边界（config/nvs_params.c:params_sanitize）
| 字段 | 钳制范围 | 说明 |
|---|---|---|
| late_ms | 500~20000 | CLI 键 late_s |
| slow_ms | 500~30000 | slow_s |
| target_speed | ≤MOTOR_SPEED_MAX(999) | target |
| motor_dir | 0~1 | dir |
| slowwins.start/dur/pct | 0~3600000ms / 200~120000ms / 5~95% | 窗口按 start 排序、重叠删除 |
| rules.len/count_req/speak_en | 1~16 / 1~10 / 0~1 | 重复词条去重 |
| count_interval_ms | 1000~60000 | ci_s |
| stop_ramp_ms | 500~15000 | sr_s |
| wait_ms | 1000~120000 | ws_s |
| led_on_ms | 500~30000 | led_s |
| dedup_ms | 500~60000 | dedup_s |
| rfid_poll_ms | 100~5000 | poll_s |
| autostop_ms | 10000~1000000 | as_s（默认 300000=5 分钟） |

## 存储/跳转常量
- PARAMS_PAGE_ADDR=0x0800FC00，PARAMS_MAGIC=0xA55A，CRC16-MODBUS（nvs_params.c:65-66）
- SYSTEM_BOOTLOADER_ADDR=0x1FFFF000（isp_jump.c:8）