# 已知问题与风险（导航）
## 注释vs代码冲突
- config.h:54 RFID_LED_POLL_MS=10"LED轮询间隔" → 实际代码用 g_params.rfid_poll_ms（默认=RFID_READ_DELAY_MS=800ms），该宏已无使用方
- docs 中"每 10ms 轮询读卡"旧描述与现实现（800ms 可调）不一致

## 已修复（历史保留）
开机 ORE 锁死；帧长>31 越界写；0xAC 误判读块；chinese_data 越界读；trig 数组[4]→MAX；上电提示音 <I>0→<I>7

## 遗留低危/新风险
| 风险 | 说明 | 等级 |
|---|---|---|
| RX 0x7F 反转义缺失 | GBK 不含 0x7F，实际不触发 | P2 |
| speak_en=0 仅取消强制播报 | 触发词仍走去重播报 | P2 |
| IDLE/RAMPUP 期触发延迟到 RUN 消费 | 上电 6s 内刷触发词先跑再停 | P2 |
| SLOW 窗口重启后可二次进入 | 绝对时间语义 | P2 |
| 绝对计时停车重启后可能立即 STOP | stop_time=g_params.autostop_ms 后仍存在（默认5分钟） | P2 |
| NONE 态无主动轮询 | 读卡依赖模块自动上报卡号响应；旧文档"200ms 低频探测"与现源码不符 | P2 |
| ISP 跳转前须拔 U13T | 其 RX 占用 USART1 干扰 Bootloader 应答（操作约束） | 操作注意 |
| CLI 写 g_params 与任务读并发 | defaultTask 改写 vs RFID/MOTOR 任务读取，复合字段更新非原子；实际低概率 | P2 |
| printf 误用于调试 | 重定向进 TTS 口会播报乱码；CLI 应答必须 Dbg_Printf | 约定