# 已知问题与风险（导航）
- 已修复（v0.3/v0.5）：开机 ORE 锁死（提前使能接收+ErrorCallback 自愈）；帧长>31 越界写；0xAC 误判读块；chinese_data 越界读（强制 0 结尾）；trig 数组 [4]→TRIGGER_RULES_MAX；100Hz 节拍 WAIT 超时漂移（改真实时间差）；NONE 态依赖自动上报失联（加 200ms 轮询）
- 遗留低危: RX 0x7F 反转义缺失（GBK 不含 0x7F）；speak_en=0 仅取消强制播报；IDLE/RAMPUP 期触发延迟到 RUN 消费；SLOW 窗口重启后可二次进入；绝对计时下停车重启后可能立即 STOP（设计既定）；UartSendCommand 转义依赖命令不含 0x7F
