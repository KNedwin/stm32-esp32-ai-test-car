# 协议分析（导航）
## U13T 读卡（USART1，权威：读卡模组使用说明书）
- 帧格式: 7F(头) + 长度 + 地址 + 命令码 + 参数 + 校验(异或)；参数中 0x7F 双写转义
- 使用命令: 读卡号 0x10/响应 0x90；读块 0x11/响应 0x91；设波特率 0x2C/响应 0xAC
- 状态码: 0x00 正确/0xFF 无卡/0xFE 错误/0xFB 校验错误
- 波特率: 默认 9600，上电发 0x2C 切 115200（模块可能记忆，本机无条件跟随）
- 读块: 单块 16 字节；块 4 有数据播块 4，空则回退块 1；不做多块拼接

## TTS（USART2 9600）
- GBK 编码直接发送；指令 <S>3/<V>6/<I>7 上电提示音7号+断电保存（rfid_task.c:37-41；原 <I>0 已改）

## USART3 参数配置行协议（config/param_cli.c，115200，\r\n 行尾）
| 命令 | 格式 | 应答 |
|---|---|---|
| HELP | HELP | 两行命令说明 |
| GET | GET all / GET <key> / GET win / GET rule | 单行 JSON / >W i s d p 多行 / >R i hex cnt spk 多行 + END |
| SET 标量 | SET late_s\|slow_s\|target\|dir\|sr_s\|ws_s\|led_s\|dedup_s\|poll_s\|ci_s\|as_s <val>(秒) | OK / ERR:key |
| SET 窗口 | SET win_add <s> <d> <p> / win_del <i> | OK / ERR:full / ERR:range |
| SET 规则 | SET rule_add <gbkhex> <cnt> <spk> / rule_del <i> | OK / ERR:full / ERR:range |
| SAVE | SAVE | OK:saved（写 Flash 末页） / ERR:save |
| DUMP | DUMP | > DUMP: 前64字节hex |
| ISP | ISP | OK:entering ISP → 软跳 Bootloader |
| REBOOT | REBOOT | OK:reboot → NVIC_SystemReset |

- 每条修改命令成功后自动 params_apply() 即时生效；应答走 Dbg_Printf（USART3），**printf 归 TTS 绝不能用于应答**
- 接收: 任务轮询 RXNE（defaultTask 10ms），行缓冲 192 字节

## PC 上位机（仓库根 tools/stm32_host/host.py）
- HTTP 127.0.0.1:8321 + pyserial 串口事务桥；参数 API 与 ESP32 网页载荷同构（秒单位 JSON）；中文触发词 PC 侧转 GBK hex
- 编译经 wsl.exe 包装调 cmake；烧录 ST-Link(st-flash) 或 ISP 软跳+stm32flash