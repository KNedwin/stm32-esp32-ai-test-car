# 协议分析
## U13T 读卡（USART1，权威：读卡模组使用说明书）
- 帧格式: 7F(头) + 长度 + 地址 + 命令码 + 参数 + 校验(异或)；参数中 0x7F 双写转义
- 使用命令: 读卡号 0x10/响应 0x90；读块 0x11/响应 0x91；设波特率 0x2C/响应 0xAC
- 状态码: 0x00 正确/0xFF 无卡/0xFE 错误/0xFB 校验错误
- 波特率: 默认 9600，上电发 0x2C 切 115200（模块可能记忆，本机无条件跟随）
- 读块: 单块 16 字节；块 4 有数据播块 4，空则回退块 1；不做多块拼接
## CN-TTS 语音（USART2 9600）
- GBK 文本直发；初始化 <S>3 语速/<V>6 音量/**<I>7 上电提示音7号**（2026-08 由 <I>0 改为与 ESP32 选定一致）
## USART3 行协议 CLI（115200，2026-08 新增，config/param_cli.c）
- 行以 \r\n 结尾；应答行以 "> " 前缀走 Dbg_Printf（单条限128B，HELP 拆两行）
- 命令: HELP / GET all\|win\|rule / SET <key> <val> / SET win_add <s> <d> <p> / win_del <i> / SET rule_add <gbkhex> <cnt> <spk> / rule_del <i> / SAVE / DUMP / ISP / REBOOT
- GET all 返回 JSON（秒单位）；GET win/rule 每项一行 "> W i s d p" / "> R i gbkhex cnt spk" + END
- 每条修改命令成功后自动 params_apply() 即时生效；SAVE 写 Flash 断电保持
## PC 上位机 API（tools/stm32_host/host.py，HTTP 127.0.0.1:8321）
- GET /api/ports·params·job；POST /api/connect·disconnect·params·reboot·cli·job(build/flash_stlink/flash_isp)
- 载荷与 ESP32 网页同构（秒单位 JSON），PC 侧中文→GBK hex；事务桥静默 0.30s 判收包完成
