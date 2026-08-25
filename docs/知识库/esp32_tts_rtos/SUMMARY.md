# SUMMARY - esp32_tts_rtos

## 一句话描述
esp32_tts_rtos：ESP32-S3 @240MHz（DevKitC-1 N16R8）读卡语音播报电机控制复刻项目（RTOS 版，双任务），U13T 读卡 + CN-TTS 播报 + 多触发词停车 + WiFi 配网参数层。（2026-08 更新）

## 核心功能
1. 晚启动 A=2s + 缓启动 B=4s 线性加速至 999
2. 实时读卡：卡在线圈上 LED 常亮，脱离 C=3s 熄灭（C/D/轮询间隔均可网页调）
3. TTS 播报：单块 16 字节 GBK；D=10s 相同内容去重；上电提示音 <I>7
4. 定时降速：默认 E=42s 起降为 F=50%，G=5s 恢复；支持多段减速窗口 slowwins[8]（网页可配）
5. 触发停车：太阳 1 次触发 / 地球 2 次触发（间隔≥10s）→ 绿色确认窗 500ms → 停车序列（H=2s 减速 + I=10s 静止 + 重启）
6. 自动停车总时长：t≥autostop_ms 永久停止，默认 5 分钟，网页可设 10~1000 秒存 NVS（原电位器方案退役）
7. WiFi 配网：连按 3 次 RST → SoftAP EV-Car-Setup(12345678) → http://192.168.4.1 改全部可调参数 → 重启生效
8. 板载 RGB（GPIO48 WS2812B）状态指示 + 数据输出口（[SYS]/[RFID]/[LED]/[MOTOR]）

## 硬件平台
| 项目 | 内容 |
|---|---|
| 芯片 | ESP32-S3 @240MHz，512KB SRAM（模组 N16R8；工程 sdkconfig 按 2MB Flash 布局） |
| 读卡模块 | U13T（UART1 GPIO10/11，9600→115200） |
| 语音模块 | CN-TTS（UART2 GPIO12/13，9600） |
| 电机 | LEDC 双通道互补 GPIO4/5（20kHz），方向可配 |
| 电位器 | **已退役**（ADC1_CH0 GPIO1 悬空无影响，adc.c 留编译） |
| LED | GPIO2/8/9（低电平亮）+ 板载 WS2812B RGB GPIO48 |
| 配网 | SoftAP EV-Car-Setup / WPA2 / 12345678 / http://192.168.4.1 |
| 调试输出 | USB-Serial-JTAG console |

## 架构图
```mermaid
graph LR
    C["config.h 参数"] --> D["驱动层"]
    D --> L["纯逻辑层(rfid_logic/motor_logic)"]
    L --> A["应用状态机"]
    A --> M["调度(main/app_main)"]
```

## 已知风险（前 5，2026-08 更新）
1. 中文路径依赖 CONFIG_LIBC_NEWLIB=y（勿删）
2. params_t 结构变更会使旧 NVS blob 尺寸不符 → 参数回落默认一次（升级需重设）
3. 绝对计时下停车重启后可能立即 STOP（设计既定）
4. rfid_task.c 头部"100Hz"注释过时（实际 sdkconfig FREERTOS_HZ=1000）；web_server.h 对 /api/restart 描述与实现有偏差
5. utf8→GBK 反查为线性扫描（仅配网保存时调用）

## 代码规模
- 业务 .c 文件：18 个（components/common 15 + main 3）；约 2100 行
- 主机单元测试：ESP32 **87 项**（rfid_logic 32 + card_parse 14 + motor_logic 30 + gbk_utf8 11）+ 两版漂移检查（编译前必跑）
