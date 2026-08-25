# 已知问题与风险（导航）
- 已修复（v0.5）：100Hz 节拍 WAIT 超时漂移 10 倍（改真实时间差）；NONE 态依赖模块自动上报失联（加 200ms 轮询）；发送 0x7F 转义分支重写；初始化 API 返回值检查
- 已修复（2026-08）：FreeRTOS tick 100→1000Hz（pdMS_TO_TICKS(1)=0 忙等/WDT 刷屏）；电位器退役（ADC 无调用方）；<I>1 误配改 <I>7
- 遗留低危: RX 0x7F 反转义缺失（GBK 不含 0x7F）；speak_en=0 仅取消强制播报；IDLE/RAMPUP 期触发延迟到 RUN 消费；SLOW 窗口重启后二次进入；绝对计时停车重启后立即 STOP（设计既定）
- 注释过时: main/rfid_task.c:36 仍写"FreeRTOS 默认 100Hz"（实际 sdkconfig 1000Hz）
- 接口注释: web_server.h 写 "POST /api/restart 保存并重启"，实现仅重启不保存（保存发生在 POST /api/params 内）
- 升级注意: params_t 结构变更 → 旧 NVS blob len 不符 → 回落默认一次（参数重设）
- run_tests.sh 段落标签 [1/3]/[2/3]/[3/4]/[4/4] 序号混排（功能不受影响）
- 中文路径陷阱: picolibc specs 不支持中文路径 → sdkconfig.defaults CONFIG_LIBC_NEWLIB=y（勿删）