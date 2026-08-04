# 已知问题与风险（导航）
- 已修复（v0.5）：100Hz 节拍 WAIT 超时漂移 10 倍（改真实时间差）；NONE 态依赖模块自动上报失联（加 200ms 轮询）；发送 0x7F 转义分支重写；初始化 API 返回值检查
- 遗留低危: RX 0x7F 反转义缺失（GBK 不含 0x7F）；speak_en=0 语义；IDLE/RAMPUP 期触发延迟；SLOW 窗口重启后二次进入；绝对计时停车重启后立即 STOP（设计既定）
- 中文路径陷阱: picolibc specs 不支持中文路径 → sdkconfig.defaults CONFIG_LIBC_NEWLIB=y（勿删）
