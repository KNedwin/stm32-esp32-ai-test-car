# 已知问题与风险（导航）
- 已修复（v0.3/v0.5）：开机 ORE 锁死（提前使能接收+ErrorCallback 自愈）；帧长>31 越界写；0xAC 误判读块；chinese_data 越界读（强制 0 结尾）；trig 数组 [4]→TRIGGER_RULES_MAX；100Hz 节拍 WAIT 超时漂移（改真实时间差）；NONE 态依赖自动上报失联（加 200ms 轮询）
- 已修复（2026-08）：上电提示音 <I>0→<I>7 与 ESP32 对齐；LED 延时/轮询间隔接入 g_params 可调
- 遗留低危: RX 0x7F 反转义缺失（GBK 不含 0x7F）；speak_en=0 仅取消强制播报；IDLE/RAMPUP 期触发延迟到 RUN 消费；SLOW 窗口重启后可二次进入；绝对计时下停车重启后可能立即 STOP（设计既定）；UartSendCommand 转义依赖命令不含 0x7F
- 新增注意点（2026-08）:
  - printf 重定向在 TTS 语音口(huart2)，调试/CLI 应答必须用 Dbg_Printf→USART3，混用会把应答播进喇叭
  - ISP 软跳前必须拔 U13T 读卡模块（其 RX 占用 USART1 干扰 Bootloader 应答）
  - params_t 结构变更会使旧 Flash blob CRC/尺寸不匹配→回落默认值一次（同 ESP32 NVS 教训）
  - ParamCli_Poll 非中断轮询，主循环被 RFID/Motor 长阻塞期间 USART3 字节可能溢出丢字（RXNE 无 FIFO）
  - Dbg_Printf 单条 128B 上限：HELP 已拆两行；**GET all 紧凑 JSON 约 140B 疑被截断至 127B（尾字段 win_n/rule_n 可能丢失，无板未验证）**；秒值整除使 <1s 字段显示为 0（如 poll_s 默认 800ms→0）
