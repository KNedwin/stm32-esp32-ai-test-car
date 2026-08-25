# 协议分析（导航）
## U13T 读卡协议（权威：读卡模组使用说明书）
- 帧格式: 7F(头) + 长度 + 地址 + 命令码 + 参数 + 校验(异或)；TX 参数中 0x7F 双写转义
- 命令: 读卡号 0x10/响应 0x90；读块 0x11/响应 0x91（仅 0x91 产生事件）；设波特率 0x2C/响应 0xAC
- 波特率: 9600→115200（card_uart.c 初始化切换）；单块 16 字节，块 4→回退块 1

## CN-TTS 语音协议（UART2 9600 GBK）
- 开机默认: <S>3 语速 / <V>6 音量 / **<I>7 上电提示音选 7 号**（<I> 指令同时启用断电保存）
- 播报: 0 结尾 GBK 字符串直发即读

## HTTP 配置协议（SoftAP 192.168.4.1，端口 80）
| 方法/路径 | 功能 |
|---|---|
| GET / | 内嵌 webpage.html 配置页 |
| GET /api/params | 当前参数 JSON（触发词 GBK→UTF-8 文本，ms 数值直出） |
| POST /api/params | JSON 提交（*_s 秒字段×1000 存 ms；rules.text UTF-8→GBK），sanitize+NVS 后 500ms 重启 |
| POST /api/restart | 直接重启 |
- POST 体上限 4096B；空闲 300s 自动 esp_restart()（config_mode_run 监控 web_server_idle_seconds）
（2026-08 更新：<I>0→<I>7 修正；新增 HTTP 协议节）