# 协议分析（U13T，权威：读卡模组使用说明书）
- 帧格式: 7F(头) + 长度 + 地址 + 命令码 + 参数 + 校验(异或)；发送侧参数中 0x7F 双写转义
- 使用命令: 读卡号 0x10/响应 0x90；读块 0x11/响应 0x91；设波特率 0x2C/响应 0xAC
- 波特率: 9600→115200（card_uart.c 初始化切换，无条件跟随）
- 读块: 单块 16 字节；块 4 → 回退块 1
- TTS: UART2 9600 GBK 直发；指令 <S>3/<V>6/<I>7（上电提示音选7号，<I> 同时启用断电保存）
- HTTP 配网协议（web_server.c）: GET /=页面(webpage.html 内嵌)；GET /api/params=当前参数 JSON(触发词转 UTF-8)；POST /api/params=JSON 提交(≤4096B，触发词 UTF-8→GBK，params_save 后 500ms esp_restart)；POST /api/restart=600ms 后重启