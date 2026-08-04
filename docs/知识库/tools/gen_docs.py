#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""第 3 层：生成 12 个知识库文档（4 项目）
共享内容（触发词矩阵/协议/参数/FAQ）参数化；平台/版本差异按项目注入
所有数值基于 v0.5 源码（已 grep 验证）
"""
import os

KB = "<HOME>/新能源小车/docs/知识库"

# ---------------- 共享内容模板 ----------------

FUNC_SPEC = """| # | 功能 | 规格 | 参数 |
|---|---|---|---|
| 1 | 晚启动 | 通电延时 A=2s 后电机才启动 | MOTOR_START_LATE_TIME_MS |
| 2 | 缓启动 | B=4s 线性加速 0→999 | MOTOR_START_SLOW_TIME_MS / MOTOR_SPEED_MAX |
| 3 | 实时读卡 + LED | 任意时刻轮询读卡；读到卡 LED 亮；卡在线圈上持续亮，脱离 C=3s 后熄灭 | LED_ON_TIME_S |
| 4 | TTS 播报去重 | 单块 16 字节 GBK 送 TTS；D=10s 内相同内容不重复播报（LED 不受影响） | SPEAK_DEDUP_TIME_S |
| 5 | 定时降速 | 第 E=42s 起速度降为 F=50%，G=5s 后恢复（绝对计时，开机窗口仅一次） | MOTOR_TIME_START_S / MOTOR_SPEED_PERCENT / MOTOR_TIME_DURATION_S |
| 6 | 触发停车 | 多触发词规则表；停车序列 = 减速 H=2s + 静止 I=10s + 重新缓启动；每个词上电周期只触发一次 | TRIGGER_RULES / TRIGGER_STOP_RAMP_TIME_S / TRIGGER_WAIT_TIME_S |
| 7 | 电位器自动停止 | 按电位器阻值线性计算停车时间（10s~600s，绝对计时） | RES_MAX / STOP_TIME_MIN_MS / STOP_TIME_MAX_MS |
| 8 | 数据输出口 | 实时输出读卡数据/电机状态/LED 状态（[SYS]/[RFID]/[LED]/[MOTOR] 格式） | DBG_ECHO_* |
| 9 | 系统保护 | 程序卡死自动重启（STM32: HardFault 复位；ESP32: 由 IDF 异常处理） | — |"""

TRIGGER_MATRIX = """| 场景 | 太阳（一次性，count_req=1） | 地球（计数型，count_req=2） |
|---|---|---|
| 第 1 次读到 | 播报 + 停车，标记"已触发" | 播报 + 亮灯，计数=1（未达次数按普通卡流程） |
| 第 2 次读到 | 正常播报亮灯（已触发，不再停车） | 距上次计数 ≥10s → 播报 + 停车，标记"已触发" |
| 触发之后 | 整个上电周期内不再触发，只播报亮灯（断电重启恢复） | 同左 |
| 10s 内重复刷 | 无影响（已触发） | 不计入次数（普通播报流程） |
| 停车序列期间 | 不计数不触发（播报流程照常） | 不计数不触发 |
| 触发时播报 | 强制播报（不受 D 秒去重约束，随后更新去重记录） | 同左 |
| LED | 始终照常亮 | 始终照常亮 |"""

U13T_PROTO = """### 帧格式（U13T 读卡模块）
`0x7F(帧头) + 长度 + 地址 + 命令码 + 参数 + 校验`；校验 = 长度^地址^命令码^参数 的异或；参数中的 0x7F 双写转义。

### 使用命令表
| 命令 | 方向 | 命令码 | 参数 |
|---|---|---|---|
| 读卡号 | → | 0x10 | 无 |
| 读卡号 | ← | 0x90 | 状态(1)+卡类型(2)+卡号(4) |
| 读块数据 | → | 0x11 | 块号(1) |
| 读块数据 | ← | 0x91 | 状态(1)+卡类型(2)+卡号(4)+数据(16) |
| 设波特率 | → | 0x2C | 波特率(4)+0x98 0x24 0x31（确认码） |
| 设波特率 | ← | 0xAC | 状态(1) |

### 状态码
| 状态码 | 意义 |
|---|---|
| 0x00 | 正确 |
| 0xFF | 无卡 |
| 0xFE | 错误或无卡 |
| 0xFB | 校验错误 |

### 读卡流程
- 单块 16 字节：先读块 4，块 4 首字节为 0 则回退读块 1；不做多块拼接
- 读块响应 20ms 无应答 → 重发；重发 2 次仍无应答 → 放弃回到无卡态
- 波特率：模块默认 9600，上电发 0x2C 命令切 115200（模块可能记忆，本机无条件跟随）

### TTS 协议（CN-TTS 模块）
- UART 9600 8N1，GBK 编码中文直接发送即播报
- 指令：`<S>3` 语速、`<V>6` 音量、`<I>0` 上电提示 + 断电保存（TTS_SetupDefaults 统一发送）

### 数据输出口协议
| 事件 | 格式 |
|---|---|
| 上电 | `[SYS] boot   LED=OFF MOTOR=STOP` |
| 读到卡 | `[RFID] <HEX 字节空格分隔>` |
| LED 变化 | `[LED] ON` / `[LED] OFF`（边沿触发） |
| 电机状态变化 | `[MOTOR] <状态> speed=<速度>`（IDLE/RAMPUP/RUN/SLOW/STOPPING/WAIT/STOP） |"""

PARAMS = """| 参数 | 值 | 可改性 | 含义 | 来源 |
|---|---|---|---|---|
| MOTOR_START_LATE_TIME_MS | 2000 | ⚙️可改 | A：通电延时后电机启动 | config.h |
| MOTOR_START_SLOW_TIME_MS | 4000 | ⚙️可改 | B：缓启动时长 | config.h |
| MOTOR_SPEED_MAX | 999 | ⚙️可改 | 电机速度上限 | config.h |
| MOTOR_TARGET_SPEED | 999 | ⚙️可改 | 目标速度 | config.h |
| MOTOR_MAX_RUN_TIME_MS | 1000000 | ⚙️可改 | 电机运行绝对上限 1000s | config.h |
| MOTOR_TIME_START_S | 42 | ⚙️可改 | E：降速窗口起点 | config.h |
| MOTOR_SPEED_PERCENT | 50 | ⚙️可改 | F：降速百分比 | config.h |
| MOTOR_TIME_DURATION_S | 5 | ⚙️可改 | G：降速窗口时长 | config.h |
| TRIGGER_RULES_MAX | 8 | 🔒不建议改 | 触发词规则数上限（数组容量） | config.h |
| TRIGGER_RULES | 太阳/地球 | ⚙️可改 | GBK 触发词表（太阳 0xCCABD1F4、地球 0xB5D8C7F2） | config.h |
| TRIGGER_COUNT_INTERVAL_MS | 10000 | ⚙️可改 | 计数型词两次有效计数最小间隔 | config.h |
| TRIGGER_STOP_RAMP_TIME_S | 2 | ⚙️可改 | H：触发停车减速过程耗时 | config.h |
| TRIGGER_WAIT_TIME_S | 10 | ⚙️可改 | I：停住后静止等待 | config.h |
| RFID_BLOCK_SIZE | 16 | 🔒不建议改 | 读卡单块字节数（S50 协议固定） | config.h |
| LED_ON_TIME_S | 3 | ⚙️可改 | C：卡脱离后 LED 熄灭延时 | config.h |
| SPEAK_DEDUP_TIME_S | 10 | ⚙️可改 | D：相同内容去重窗口 | config.h |
| RFID_READ_DELAY_MS | 800 | ⚙️可改 | 播报后到下次读卡的间隔 | config.h |
| RFID_LED_POLL_MS | 10 | ⚙️可改 | LED 保持期轮询读卡间隔 | config.h |
| RFID_READ_TIMEOUT_MS | 20 | ⚙️可改 | 读块响应超时 | config.h |
| RES_MAX | 5000 | ⚙️可改 | 电位器阻值量程 | config.h |
| STOP_TIME_MIN_MS | 10000 | ⚙️可改 | 停车时间下限 | config.h |
| STOP_TIME_MAX_MS | 600000 | ⚙️可改 | 停车时间上限 | config.h |
| DBG_USART_ENABLE | 1 | ⚙️可改 | 数据输出口总开关 | config.h |
| DBG_ECHO_RFID / MOTOR / LED | 1 | ⚙️可改 | 分类输出开关 | config.h |
| RFID_READ_DATA_WHEN_START | 1 | ⚙️可改 | 开机即尝试读卡 | config.h |
| RFID_SETTING_SPEAK_SPEED | 1 | ⚙️可改 | 开机设置语速/音量/保存 | config.h |

> 修改 config.h 后重新编译即可；触发词换词方法：`python -c "print(' '.join('\\\\x%02X' % b for b in '新词'.encode('gbk')))"` 得到字节串填入 TRIGGER_RULES 并同步 len。"""

FAQ_ITEMS = [
    ("构建与烧录", "如何编译并烧录程序？",
     "STM32：在工程目录执行 `cmake -B build && cmake --build build`，用 `st-flash write build/<工程名>.bin 0x08000000` 烧录。ESP32：先 `source <HOME>/.espressif/tools/activate_idf_v6.0.2.sh` 激活环境，再 `idf.py build`、`idf.py -p /dev/ttyACM0 flash monitor`。"),
    ("构建与烧录", "编译报 `cannot read spec file`（仅 ESP32）？",
     "ESP-IDF 默认 picolibc 的 gcc specs 机制不支持中文路径。本项目 sdkconfig.defaults 已配置 `CONFIG_LIBC_NEWLIB=y` 绕开，请勿删除该配置。"),
    ("构建与烧录", "CubeMX 重新生成工程后编译报错或行为异常（仅 STM32）？",
     "重新生成会覆盖 4 处生成代码：USART2 波特率（须 9600）、TIM2 CH2（须 PWM2 + OCFastMode ENABLE）、ADC 采样时间（须 28.5 cycles）、FreeRTOSConfig.h 堆大小（须 8192）。业务代码在 hardware/Task/config.h 与 USER CODE 区，不受影响。"),
    ("构建与烧录", "任务栈溢出/FreeRTOS 内存不足怎么调（仅 STM32 RTOS 版）？",
     "RFID_Task 栈 1024 字、Motor 栈 512 字、堆 8192。若链接报 RAM 超限应减小堆或栈；若运行时任务创建失败（xTaskCreate 返回 NULL）才增大堆。"),
    ("读卡与协议", "读卡没反应怎么办？",
     "① 确认波特率切换代码在 usart.c（STM32）/card_uart.c（ESP32）的初始化中执行（9600 发 0x2C → 115200）；② 读卡模块供电 3.0~5.5V 并共地；③ 卡片在感应区；④ 检查帧解析：模块响应 0x7F 开头，长度≤31。"),
    ("读卡与协议", "为什么开机波特率切换时读卡可能失效（STM32 版已修复）？",
     "9600 期模块 0xAC 响应若在接收中断使能前到达会造成 ORE 溢出锁死。修复：初始化先使能接收（清乱码）再发切换命令，并实现 HAL_UART_ErrorCallback 自愈。ESP32 版 IDF 驱动环形缓冲天然免疫。"),
    ("读卡与协议", "帧长度字段超过 31 会怎样？",
     "解析器会丢弃整帧并复位状态机（长度上限防护），不会越界写 ReceiveBuffer[32]。"),
    ("读卡与协议", "卡数据包含 0x7F 字节会怎样？",
     "发送侧做了 0x7F 双写转义；接收侧未做反转义（遗留低危项）。GBK 汉字字节范围 0x81~0xFE 不含 0x7F，正常中文卡数据不会触发。"),
    ("触发词与去重", "如何添加新的触发词？",
     "改 config.h 的 TRIGGER_RULES 表：用 Python 计算 GBK 编码（如 `\"火星\".encode('gbk')` → \\xBB\\xF0\\xD0\\xC7），追加一行 `{ (const uint8_t*)\"\\xBB\\xF0\\xD0\\xC7\", 4, 1, 1 }`，同步 len；条数上限 TRIGGER_RULES_MAX=8。"),
    ("触发词与去重", "为什么\"太阳\"只触发一次，之后只播报？",
     "设计为一次性触发词（count_req=1）：第 1 次读到播报+停车并置 triggered 标志，整个上电周期内不再触发，断电重启才恢复。"),
    ("触发词与去重", "为什么\"地球\"需要读两次才触发？",
     "地球是计数型触发词（count_req=2）：第 1 次只播报亮灯并计数，第 2 次（距上次计数 ≥10s）才播报+停车；10s 内连刷不计次数。"),
    ("触发词与去重", "D 秒去重是什么？为什么触发词不受去重限制？",
     "D=10s 内相同内容不重复播报（普通卡）。触发词的播报是强制播报（不受去重约束），因为触发事件必须播报；播报后会更新去重记录。"),
    ("电机控制", "电机不转或方向不对？",
     "① STM32：确认 TIM2 两通道都是 PWM Mode 2（CH2 常被生成成 PWM1，需手工改）；② 检查 Motor_Control 参数 0~999；③ 电机驱动板供电；④ ESP32 检查 LEDC GPIO4/5 接线。"),
    ("电机控制", "缓启动/减速不平滑（一顿一顿）？",
     "属正常现象：STM32 裸机版 TTS 发送（9600 单块 16 字节约 17ms）期间主循环停顿；ESP32 版 100Hz 节拍 vTaskDelay(1)=10ms 步进。功能不受影响，可增大缓启动时长 B 更柔。"),
    ("电机控制", "停车重启后立即 STOP（再也不转）？",
     "这是绝对计时的边界行为：电位器 stop_time 是上电后总时长，若停车序列（H+I=12s）+ 已运行时间越过 stop_time，重启进 RUN 的瞬间即 STOP。若需要重启后再跑，需改相对计时（设计决策）。"),
    ("电机控制", "E=42s 降速窗口为什么重启后可能再次降速？",
     "窗口按绝对时间 t∈[42s,47s) 判定；若触发停车重启后 t 仍落在窗口内（触发发生在 26~31s 区间），会再次降速到窗口结束。这是绝对时间语义的合理行为。"),
    ("电机控制", "电位器如何调节停车时间？",
     "电位器接 ADC（STM32 PB1 / ESP32 GPIO1），上电采样 20 次平均后线性映射：adc=4095 → 10s（最短），adc=0 → 600s（最长）。公式含 clamp 保护。"),
    ("LED 与调试", "LED 为什么卡在线圈上一直亮、移开后不立即灭？",
     "设计如此：卡在线圈上时读卡号响应持续刷新卡在场时间戳，LED 常亮；移开卡后约 C=3s（LED_ON_TIME_S）熄灭。"),
    ("LED 与调试", "数据输出口看不到输出/乱码？",
     "STM32：USART3 115200（screen /dev/ttyUSB0 115200）；ESP32：USB-Serial-JTAG console（idf.py monitor）。检查波特率与接线交叉；LED/电机输出为边沿触发（状态变化才输出一行），读卡数据每条输出。"),
    ("LED 与调试", "调试输出影响实时性吗？",
     "输出口每行 <40 字节：STM32 USART3 115200 约 3ms、ESP32 console 微秒级，影响可忽略；RTOS 版两任务并发输出时可能丢一条（HAL 返回 BUSY），不影响功能。"),
]

def _param_groups():
    groups = {
        "1. 电机时序与降速窗口": [],
        "2. 触发词规则": [],
        "3. LED 与去重": [],
        "4. 电位器自动停止": [],
        "5. 调试输出与其他": [],
    }
    header_done = False
    for line in PARAMS.strip().split("\n"):
        if line.startswith("| 参数"):
            header_done = True
            continue
        if not line.startswith("| "):
            continue
        if not header_done:
            continue
        key = line.split("|")[1].strip()
        if key.startswith("TRIGGER"):
            groups["2. 触发词规则"].append(line)
        elif key.startswith(("LED_ON_TIME", "SPEAK_DEDUP", "RFID_READ_DELAY", "RFID_LED_POLL", "RFID_READ_TIMEOUT", "RFID_BLOCK")):
            groups["3. LED 与去重"].append(line)
        elif key.startswith(("RES_MAX", "STOP_TIME")):
            groups["4. 电位器自动停止"].append(line)
        elif key.startswith(("DBG_", "RFID_READ_DATA", "RFID_SETTING")):
            groups["5. 调试输出与其他"].append(line)
        else:
            groups["1. 电机时序与降速窗口"].append(line)
    return groups

def _params_md(extra=None):
    md = "# 关键参数表\n\n> 修改 config.h 后重新编译；触发词换词方法见 01_项目介绍 §3。\n\n"
    for title, rows in _param_groups().items():
        md += f"## {title}\n\n"
        md += "| 参数 | 值 | 可改性 | 含义 | 来源 |\n|---|---|---|---|---|\n"
        md += "\n".join(rows) + "\n\n"
    if extra:
        md += extra
    return md

def _faq_md():
    md = "# 常见问题清单\n\n> 20 个 Q&A，5 类各 4 个\n\n"
    nums = {"构建与烧录": "1", "读卡与协议": "2", "触发词与去重": "3", "电机控制": "4", "LED 与调试": "5"}
    prev = None
    for i, (cls, q, a) in enumerate(FAQ_ITEMS):
        if cls != prev:
            md += f"## {nums[cls]}. {cls}\n\n"
            prev = cls
        md += f"### Q{i+1}（{cls}）：{q}\n**A**：{a}\n\n"
    return md

def gen_doc(name, docs):
    d = f"{KB}/{name}"
    os.makedirs(d, exist_ok=True)
    for fname, content in docs.items():
        with open(f"{d}/{fname}", "w", encoding="utf-8") as f:
            f.write(content)
    print(f"[OK] {name}: {len(docs)} 文档")

# ---------------- STM32 文档 ----------------
def stm32_docs(name, is_rtos):
    plat = "STM32" 
    rt = "RTOS 版" if is_rtos else "裸机版"
    led_out = "USART3 115200（GPIO PB10/PB11）"
    build = "cmake + arm-none-eabi-gcc；st-flash 烧录 0x08000000"
    hw = f"""# 硬件配置

## 1. 主控芯片
- **芯片**: STM32F103C8T6（Cortex-M3 @72MHz，64KB Flash / 20KB RAM）✅代码确认
- **系统时钟**: HSE 8MHz × PLL9 = 72MHz；AHB /1；APB1 /2（36MHz）；APB2 /1（72MHz）；ADC /6（12MHz）
- **HAL 时基**: {"TIM1（FreeRTOS 占用 SysTick）" if is_rtos else "SysTick"}

```mermaid
flowchart LR
    HSE["HSE 8MHz(PD0/1)"] --> PLL["PLL x9"]
    PLL --> SYSCLK["SYSCLK 72MHz"]
    SYSCLK --> AHB["AHB 72MHz /1"]
    AHB --> APB1["APB1 36MHz /2"]
    AHB --> APB2["APB2 72MHz /1"]
    APB1 --> TIM2["TIM2 72MHz(x2)"]
    APB2 --> ADC["ADC 12MHz(/6)"]
```

## 2. 外设资源使用总表
| 外设 | 用途 | 关键参数 | 确认度 |
|---|---|---|---|
| USART1 | U13T 读卡模块 | 9600→115200，8N1，RX 中断 | ✅代码确认 |
| USART2 | CN-TTS 语音模块 | 9600，8N1，printf 重定向 | ✅代码确认 |
| USART3 | 数据输出口 | 115200，8N1 | ✅代码确认 |
| TIM2 | 电机 PWM | 1kHz，PSC=71，ARR=999，PWM2 双通道 | ✅代码确认 |
| ADC1 | 电位器 | IN9，28.5 cycles，连续转换 | ✅代码确认 |
| GPIO | LED×3 | 推挽+上拉，低电平亮 | ✅代码确认 |
| SWD | 调试下载 | PA13/PA14 | ✅代码确认 |
| HSE | 8MHz 晶振 | PD0/PD1 | ✅代码确认 |

## 3. GPIO 引脚分配表
| 引脚 | 功能 | 配置 | 确认度 |
|---|---|---|---|
| PA0 | TIM2_CH1 电机 PWM | AF_PP，低速 | ✅代码确认 |
| PA1 | TIM2_CH2 电机 PWM | AF_PP，低速 | ✅代码确认 |
| PA2 | USART2_TX（TTS） | AF_PP，高速 | ✅代码确认 |
| PA3 | USART2_RX（TTS） | 输入，无上下拉 | ✅代码确认 |
| PA8 | LED（低电平亮） | 推挽输出+上拉，初始高 | ✅代码确认 |
| PA9 | USART1_TX（读卡） | AF_PP，高速 | ✅代码确认 |
| PA10 | USART1_RX（读卡） | 输入，无上下拉 | ✅代码确认 |
| PA13/PA14 | SWDIO/SWCLK | Serial Wire | ✅代码确认 |
| PB1 | ADC1_IN9（电位器） | 模拟输入 | ✅代码确认 |
| PB10 | USART3_TX（调试输出） | AF_PP，高速 | ✅代码确认 |
| PB11 | USART3_RX（调试输出） | 输入，无上下拉 | ✅代码确认 |
| PB12 | LED | 推挽输出+上拉，初始高 | ✅代码确认 |
| PC13 | LED | 推挽输出+上拉，初始高 | ✅代码确认 |
| PD0/PD1 | OSC_IN/OUT（HSE 8MHz） | 晶振 | ✅代码确认 |

> 接线注意：TTS 模块必须 5V 供电（播报峰值 320mA）；U13T 供电 3.0~5.5V；串口 TX/RX 交叉；共地。
"""
    arch = f"""# 系统架构

## 1. 总体分层
```mermaid
graph LR
    C["config.h 参数"] --> D["hardware/ 驱动"]
    D --> L["Task/ 纯逻辑(rfid_logic/motor_logic)"]
    L --> A["Task/ 应用状态机"]
    A --> M["main.c 调度"]
```
- config.h（纯参数）→ hardware/（HAL 驱动）→ 纯逻辑层（无硬件依赖，四版共享）→ 应用状态机 → main 调度
- 依赖单向，无循环；纯逻辑层（rfid_logic/motor_logic）与 STM32/ESP32 各版逐字相同，主机可测

## 2. 启动流程
```mermaid
flowchart TD
    S["上电"] --> H["HAL_Init + SystemClock_Config"]
    H --> P["MX_GPIO/ADC1/TIM2/USART1/2/3 初始化"]
    P --> B["USART1 波特率切换(9600→115200, 先使能接收防ORE)"]
    B --> I["LED_Sta(0) / PWM_Init / Motor_Control(0) / Dbg_Init"]
    I --> T["TTS 设置(语速/音量/保存)"]
    T --> R["RFID 参数初始化 + 卡状态 EXIST"]
    R --> LOOP["运行时调度"]
"""
    if is_rtos:
        arch += """
## 3. 任务模型（FreeRTOS CMSIS_V1）
| 任务 | 优先级 | 栈 | 职责 | 文件 |
|---|---|---|---|---|
| RFID_Task | High(2) | 1024 字 | 读卡五态状态机/播报/LED/触发 | Task/rfid_task.c |
| Motor_Control_Task | Idle(-3) | 512 字 | 电位器采样 + 喂 motor_logic + PWM 输出 | Task/motor_control_task.c |
| defaultTask | Normal | 128 字 | 空转（CubeMX 遗留，无业务） | Core/Src/freertos.c |

- 任务间通信：全局 volatile 标志（card_res_flag 由 ISR 置位任务消费；motor_trigger_flag 由 RFID 置位电机消费）
- RFID_Task 高优先级保证读卡及时；电机任务 1ms 一拍（vTaskDelay(1)）
"""
    else:
        arch += """
## 3. 主循环模型（裸机）
```c
while(1) {
    RFID_Process();   /* 读卡/播报/LED/触发（非阻塞状态机） */
    Motor_Process();  /* 电机时序（非阻塞状态机，喂 motor_logic） */
}
```
- 所有延时用 HAL_GetTick() 时间差非阻塞实现；TTS 发送（约 17ms）期间两状态机停顿，缓启动跳档毫秒级可接受
"""
    arch += """
## 4. 状态机
### 读卡五态（card_res_flag）
```mermaid
stateDiagram-v2
    [*] --> NONE: 上电
    NONE --> EXIST: 读到卡号
    EXIST --> WAIT: 发读块命令
    WAIT --> EXIST: 20ms 超时重发(2次后放弃)
    WAIT --> RESDATA: 收到读块响应
    RESDATA --> LEDLIGHT: 数据有效→处理(触发/去重/播报)
    LEDLIGHT --> NONE: 卡脱离 C 秒后灭灯
    LEDLIGHT --> EXIST: 再次读到卡号
```
### 电机状态机（motor_logic，绝对计时）
```mermaid
stateDiagram-v2
    [*] --> IDLE
    IDLE --> RAMPUP: t≥A(2s)
    RAMPUP --> RUN: t≥A+B
    RUN --> SLOW: t∈[E,E+G) 降速F%
    SLOW --> RUN: t≥E+G
    RUN --> STOPPING: 触发停车
    STOPPING --> WAIT: 减速 H=2s 到 0
    WAIT --> RAMPUP: 静止 I=10s 后重启(不再经晚启动)
    RUN --> STOP: t≥stop_time 或 1000s
```
"""
    mods = f"""# 功能模块

## 1. 读卡协议驱动（hardware/rfid_card/Card.c|h）
- **职责**: U13T 模块命令组帧/发送、响应帧逐字节解析、波特率切换、接收中断回调
- **对外 API**: SetBound115200()、ReadCard()、ReadBlock(block)、UartReceiveCommand(data)、HAL_UART_RxCpltCallback、HAL_UART_ErrorCallback
- **关键逻辑**:
  - 帧解析状态机：0x7F 帧头 → 长度(≤31 防护) → 命令码/参数 → 仅 0x91（读块）产生事件、0x90 分"读到卡号/无卡"、0xAC 忽略
  - 回调：temp_res==2 在 LEDLIGHT 态刷新 rfid_last_card_tick（LED 常亮依据）；temp_res==3 不再主动熄灯
  - ErrorCallback：ORE 溢出自愈（清标志 + 重装接收中断）

## 2. 触发/去重逻辑层（Task/rfid_logic.c|h，四版共享）
- **职责**: 触发词匹配（子串搜索）、一次性/计数型计数（首次数放行 + 10s 间隔）、去重判断、事件输出
- **对外 API**: RfidLogic_Init/Process/TriggerMatch/IsDup/UpdateSpeak/RuleCount
- **事件**: RFID_EV_NONE/SPEAK/SPEAK_FORCED/TRIGGER_STOP（位掩码）

## 3. 电机状态机逻辑层（Task/motor_logic.c|h，四版共享）
- **职责**: 绝对计时状态机（IDLE/RAMPUP/RUN/SLOW/STOPPING/WAIT/STOP）、电位器→stop_time 换算
- **对外 API**: MotorLogic_Init/Step/IsInStopSequence/CalcStopTime/StateName
- **pending_trigger**: IDLE/RAMPUP 期触发挂起，RUN/SLOW 消费

## 4. 读卡应用状态机（Task/{"rfid_task.c" if is_rtos else "rfid_process.c"}）
- **职责**: 五态流转（EXIST→WAIT→RESDATA→LEDLIGHT→NONE）、播报、LED 保持（tick 法）、触发标志置位
- 单块读取（块4→回退块1）；播报缓冲统一 chinese_data[16] 并强制 0 结尾

## 5. 电机应用层（Task/{"motor_control_task.c" if is_rtos else "motor_process.c"}）
- **职责**: 电位器 20 次采样 → MotorLogic_CalcStopTime → 每拍喂 MotorLogic_Step → Motor_Control 输出；状态变化边沿调试输出

## 6. TTS 发送（hardware/USART/BSP_USART.c|h）
- **职责**: printf 重定向（__io_putchar 强定义 → USART2）、Usartx_SendString
- 注意：printf 归 TTS（语音），调试输出请用 Dbg_Printf（USART3）

## 7. 电机驱动（hardware/pwm/PWM.c|h）
- **职责**: TIM2 双通道 PWM2 启动；Motor_Control(speed 0~999)
- 映射：speed=0 → 双路 CCR=999（同电位停）；speed>0 → CH1=0、CH2=speed（差分电压 = speed/999×3.3V）

## 8. LED（hardware/LED/led.c|h）
- **职责**: 三引脚（PC13/PB12/PA8）低电平亮；边沿触发调试输出（防 10ms 轮询刷屏）

## 9. 电位器采样（hardware/ADC/BSP_ADC.c|h）
- **职责**: Get_ADC_Value() 单次转换；公式（含 clamp）在 motor 层

## 10. 数据输出口（hardware/DEBUG/Debug.c|h）
- **职责**: Dbg_Init（[SYS] boot）、Dbg_Printf（vsnprintf → USART3）；局部缓冲可重入
"""
    proto = f"""# 通信协议

## 1. U13T 读卡协议
{U13T_PROTO.split('### TTS 协议')[0]}
## 2. TTS 语音协议
{U13T_PROTO.split('### TTS 协议')[1].split('### 数据输出口协议')[0]}
## 3. 数据输出口协议
{U13T_PROTO.split('### 数据输出口协议')[1]}
"""
    params = _params_md()

    issues = f"""# 已知问题与建议

## P0 严重（已修复，保留记录）
### P0-001: 开机波特率切换期 ORE 溢出致读卡永久失效
- **状态**: 已修复（v0.3）
- **文件**: Core/Src/usart.c（USART1_Init 2 区）、hardware/rfid_card/Card.c
- **现象**: 9600 期模块 0xAC 响应先于接收中断使能到达，RXNE 未被读走 → ORE → 读卡静默失效
- **修复**: ① 切换命令前先 HAL_UART_Receive_IT（乱码被解析器丢弃）② HAL_UART_ErrorCallback 自愈重装

### P0-002: 帧长度字段无上限校验 → ReceiveBuffer[32] 越界写
- **状态**: 已修复（v0.3）
- **文件**: hardware/rfid_card/Card.c:UartReceiveCommand
- **修复**: 长度 >31 直接丢弃整帧并复位状态机

### P0-003: 0xAC 响应误判为读块数据
- **状态**: 已修复（v0.3）
- **文件**: hardware/rfid_card/Card.c
- **修复**: 仅 0x91 产生 RESDATA 事件；0xAC/0x90 其他响应不产生事件

## P1 重要（已修复，保留记录）
### P1-001: 触发词状态数组硬编码 [4]，规则超 4 条越界
- **状态**: 已修复（v0.3）
- **修复**: config.h 新增 TRIGGER_RULES_MAX=8，数组按宏定义

### P1-002: 播报缓冲 16 字节全非零时越界读
- **状态**: 已修复（v0.3）
- **修复**: chinese_data 拷贝后强制 [15]=0 结尾

## P2 遗留（低风险，建议后续处理）
| 编号 | 问题 | 影响 | 建议 |
|---|---|---|---|
| P2-001 | RX 方向 0x7F 反转义缺失 | 卡数据含 0x7F 时帧错位（GBK 汉字不含 0x7F，实际不触发） | 解析时双字节合并 |
| P2-002 | speak_en=0 语义：仅取消强制播报，触发词仍会去重播报 | 配置 speak_en=0 时行为与"不播报"直觉不符 | 文档说明或改三态 |
| P2-003 | IDLE/RAMPUP 期触发延迟到 RUN 才消费 | 上电 6s 内刷触发词，车先跑起来才停 | RAMPUP 分支检查 pending_trigger |
| P2-004 | SLOW 窗口重启后可二次进入 | 触发在 26~31s 时重启后仍处窗口内继续降速 | 加"SLOW 已用"持久标志 |
| P2-005 | 绝对计时：停车重启后可能立即 STOP | 电位器小 + 触发早时重启即停（设计既定） | 确认规则后改相对计时 |
| P2-006 | UartSendCommand 转义依赖命令不含 0x7F | 当前命令字节固定不含，安全 | 加编译期断言 |
| P2-007 | card_res_flag 未声明 volatile（理论风险） | 实际安全（中断回调置位，任务轮询） | 已声明 volatile（v0.3 修复） |
| P2-008 | 重发"2 次后放弃"实际只重发 1 次（STM32） | 模块 2ms 内正常响应，无实际影响 | 计数语义统一 |
"""
    dataflow = f"""# 数据流与控制流

## 1. 读卡数据流
```mermaid
flowchart TD
    U["USART1 RX 中断"] --> P["HAL_UART_RxCpltCallback(Card.c)"]
    P --> F["UartReceiveCommand 帧解析"]
    F --> FLAG["card_res_flag 置位"]
    FLAG --> SM["RFID 状态机(EXIST→WAIT→RESDATA)"]
    SM --> D["chinese_data[16]"]
    D --> LG["RfidLogic_Process(匹配/计数/去重)"]
    LG -->|EV_SPEAK/SPEAK_FORCED| TTS["TTS 播报(USART2) + UpdateSpeak"]
    LG -->|EV_TRIGGER_STOP| TF["motor_trigger_flag=1"]
    LG -->|EV_NONE| SKIP["去重拦截不播报(LED照常)"]
```
## 2. 电机控制流
```mermaid
flowchart TD
    ADC["电位器 ADC 采样×20"] --> CT["MotorLogic_CalcStopTime"]
    CT --> S["MotorLogic_Step(绝对计时状态机)"]
    TF["motor_trigger_flag"] --> S
    S --> MC["Motor_Control(speed)"]
    MC --> PWM["TIM2 双通道 PWM2"]
```
## 3. 触发停车完整链路
读卡 → rfid_logic 命中规则（太阳第 1 次/地球第 2 次且间隔≥10s）→ EV_TRIGGER_STOP → motor_trigger_flag → MotorLogic_Step 消费 → STOPPING(H=2s 线性减速) → WAIT(I=10s 静止) → RAMPUP(重启缓启动，不再经晚启动) → RUN
## 4. LED 控制流
读到卡 → LEDLIGHT：LED_Sta(1) → 播报后 800ms → 每 10ms 轮询 ReadCard() → 卡在场回调刷新 rfid_last_card_tick → 脱离后 now-last_tick ≥ C=3s → LED_Sta(0) 回 NONE
"""
    struct = f"""# 项目结构总览

## 1. 目录结构
```
stm32/{name}/
├── config.h                    # 全部可调参数（触发词表/时序/开关）
├── Core/
│   ├── Inc/                    # CubeMX 生成头文件
│   └── Src/                    # main.c/usart.c/tim.c/adc.c/gpio.c/freertos.c(RTOS版)/stm32f1xx_it.c
├── Drivers/                    # HAL 库 + CMSIS（CubeMX 生成，勿覆盖）
{"├── Middlewares/                   # FreeRTOS 源码（RTOS 版）" if is_rtos else ""}
├── hardware/                   # 业务驱动
│   ├── USART/                  # BSP_USART（__io_putchar→USART2）
│   ├── rfid_card/              # Card.c|h（U13T 协议）
│   ├── pwm/                    # PWM.c|h（电机）
│   ├── LED/                    # led.c|h
│   ├── ADC/                    # BSP_ADC.c|h
│   └── DEBUG/                  # Debug.c|h（USART3 输出）
├── Task/                       # 业务逻辑
│   ├── {"rfid_task.c|h / motor_control_task.c|h / freertos.c 任务创建" if is_rtos else "rfid_process.c|h / motor_process.c|h"}
│   ├── rfid_logic.c|h          # 触发/去重纯逻辑（四版共享）
│   └── motor_logic.c|h         # 电机状态机纯逻辑（四版共享）
├── CMakeLists.txt              # 根构建（业务源码在此，CubeMX 重生成不覆盖）
├── cmake/stm32cubemx/          # CubeMX 生成构建物（重生成会覆盖，勿手改）
└── build/                      # 编译产物（gitignore）
```
## 2. 文件地图
| 文件 | 职责 | 关键函数 |
|---|---|---|
| main.c | 初始化 + 调度 | main/SystemClock_Config |
| usart.c | 三串口配置 + 波特率切换 | MX_USART1/2/3_UART_Init |
| tim.c | TIM2 PWM | MX_TIM2_Init |
| adc.c | ADC1 | MX_ADC1_Init |
| Card.c | 读卡协议 | UartReceiveCommand |
| rfid_logic.c | 触发/去重 | RfidLogic_Process |
| motor_logic.c | 电机状态机 | MotorLogic_Step |
| {"rfid_task.c" if is_rtos else "rfid_process.c"} | 读卡状态机 | {"RFID_Task" if is_rtos else "RFID_Process"} |
| {"motor_control_task.c" if is_rtos else "motor_process.c"} | 电机应用 | {"Motor_Control_Task" if is_rtos else "Motor_Process"} |
| BSP_USART.c | printf 重定向 | __io_putchar |
| PWM.c | 电机驱动 | Motor_Control |
| led.c | 灯控 | LED_Sta |
| Debug.c | 数据输出 | Dbg_Printf |

## 3. 关键依赖与注意事项
- **CubeMX 重新生成会覆盖 4 处**：USART2=9600、TIM2 CH2=PWM2、ADC 28.5 cycles、FreeRTOSConfig.h heap=8192（RTOS 版）
- **业务代码**：config.h/hardware/Task 独立目录 + USER CODE 区，重生成不受影响
- **printf 归 TTS（USART2）**，调试输出用 Dbg_Printf（USART3），勿混用
- **测试**：stm32/tests/run_tests.sh（Card 25 + rfid_logic 32 + motor_logic 30 + 仿真 22）
"""
    sem = f"""# 代码语义化

> 核心函数卡片（优先级 1 文件全部导出函数）

## rfid_logic.c（触发/去重纯逻辑）
| 函数 | 语义 |
|---|---|
| RfidLogic_Init | 清零触发计数/去重状态（上电调用一次） |
| RfidLogic_TriggerMatch | 在 data[len] 中逐规则子串搜索触发词；命中返回规则号，未命中 -1；空词条/超长词条防御 |
| RfidLogic_IsDup | last_speak 与本次内容相同且距上次播报 <D 秒 → 返回 1 |
| RfidLogic_UpdateSpeak | 播报后拷贝内容并记录时刻（去重基准） |
| RfidLogic_Process | 核心决策：匹配→已触发/停车序列→普通去重播报；count_req=1 直接触发；count_req>1 首次数放行+10s 间隔计数，达次触发；返回事件位掩码 |
| RfidLogic_RuleCount | 由 TRIGGER_RULES 表 sizeof 推导条数 |

## motor_logic.c（电机状态机纯逻辑）
| 函数 | 语义 |
|---|---|
| MotorLogic_Init | 记录绝对计时起点/目标速度/stop_time，状态置 IDLE |
| MotorLogic_Step | 每拍状态机推进：pending_trigger 并入；IDLE→RAMPUP(缓启动 B 秒)→RUN；RUN/SLOW 内消费触发→STOPPING(H 秒线性减速)→WAIT(I 秒静止)→RAMPUP 重启；t≥stop_time/1000s→STOP；返回本拍目标速度 |
| MotorLogic_IsInStopSequence | state∈{{STOPPING,WAIT}} 返回 1（供 rfid 不计数） |
| MotorLogic_CalcStopTime | 电位器均值→停车时间线性插值（clamp：adc≥4095→最短，负值钳 0，上限 RES_MAX） |
| MotorLogic_StateName | 状态枚举→字符串（调试输出） |

## {"rfid_task.c（读卡任务）" if is_rtos else "rfid_process.c（读卡状态机）"}
| 函数 | 语义 |
|---|---|
| {"RFID_Task" if is_rtos else "RFID_Init + RFID_Process"} | 读卡五态流转：EXIST 发读块→WAIT 20ms 超时重发(2 次后放弃)→RESDATA 单块处理→LEDLIGHT 轮询保持→NONE 低频探测；数据经 RfidLogic_Process 决策后置触发标志/播报 |
| RFID_HandleCardData | 调用 RfidLogic_Process 并执行事件动作（置 motor_trigger_flag / TTS 播报） |
| RFID_Speak | 送 TTS 并更新去重记录 |
| BufClear | 清缓冲（上限 16 字节，防越界） |

## {"motor_control_task.c（电机任务）" if is_rtos else "motor_process.c（电机状态机）"}
| 函数 | 语义 |
|---|---|
| {"Motor_Control_Task" if is_rtos else "Motor_Init + Motor_Process"} | 电位器 20 次采样→CalcStopTime→每拍 MotorLogic_Step→Motor_Control；状态变化边沿 Dbg_Printf |
| Motor_IsInStopSequence | 转发 MotorLogic_IsInStopSequence |

## Card.c（读卡协议）
| 函数 | 语义 |
|---|---|
| SetBound115200 | 组 0x2C 命令帧（9600 发送）切换模块波特率 |
| ReadCard / ReadBlock | 组 0x10/0x11 命令帧并发送（含 0x7F 转义） |
| UartReceiveCommand | 逐字节帧解析状态机（帧头/长度≤31/命令码/参数）；返回 0/1/2/3 |
| HAL_UART_RxCpltCallback | 按解析结果置 card_res_flag；LEDLIGHT 态读到卡号刷新 rfid_last_card_tick |
| HAL_UART_ErrorCallback | ORE 清标志 + 重装接收中断（自愈） |
"""
    faq = _faq_md()
    read = f"""# 阅读指南

## 项目一句话
"{name}"：STM32F103C8T6 {rt}，实现 U13T 读卡 + CN-TTS 语音播报 + 电机控制（多触发词停车/定时降速/电位器停止）+ LED + 数据输出口。

## 文档地图
| 文档 | 内容 | 适合谁 |
|---|---|---|
| 01_项目介绍 | 功能清单/硬件组成/技术栈 | 所有人 |
| 02_硬件配置 | 芯片/时钟/外设/引脚 | 硬件调试 |
| 03_系统架构 | 分层/启动流程/状态机 | 架构理解 |
| 04_功能模块 | 10 个业务模块逐一说明 | 功能开发 |
| 05_通信协议 | U13T 帧协议/TTS/输出协议 | 联调 |
| 06_关键参数表 | config.h 全参数 | 调参 |
| 07_已知问题与建议 | P0-P2 问题清单 | 维护 |
| 08_数据流与控制流 | 读卡/电机/触发/灯控链路 | 排障 |
| 09_项目结构总览 | 目录树/文件地图 | 新人 |
| 10_代码语义化 | 核心函数卡片 | 改代码前 |
| 11_常见问题清单 | 20 个 Q&A | 快速求助 |

## 快速上手路径
1. 新人: 01 → 09 → 03
2. 调参: 06 → 01
3. 排障: 11 → 08 → 07
4. 开发: 04 → 10

## 代码导航速查
- 全部参数: config.h
- 触发词行为: Task/rfid_logic.c
- 电机时序: Task/motor_logic.c
- 读卡状态机: Task/{"rfid_task.c" if is_rtos else "rfid_process.c"}
- 协议解析: hardware/rfid_card/Card.c
- 构建: 见 01_项目介绍 §构建
"""
    intro = f"""# 项目介绍

## 1. 项目概述
{name} 是"读卡语音播报程序源码 4.0 案例"的复刻工程（{rt}，芯片 STM32F103C8T6），用于新能源小车比赛场景：刷 NFC 卡（U13T 模块）→ TTS 播报卡内 GBK 中文 + 电机联动（触发词停车/定时降速/电位器停止）。

## 2. 功能清单
{FUNC_SPEC}

## 3. 触发词行为矩阵（功能 6 详解）
{TRIGGER_MATRIX}

> 停车序列：触发 → STOPPING（H=2s 从当前速度线性减速到 0）→ WAIT（I=10s 静止）→ RAMPUP（重新缓启动 B=4s，不再经过晚启动）→ RUN。触发到重启共 H+I=12s。

## 4. 硬件组成
| 部件 | 型号/规格 | 接口 |
|---|---|---|
| 主控 | STM32F103C8T6（64KB/20KB） | — |
| 读卡模块 | U13T（13.56MHz，ISO14443A/B） | USART1 9600→115200 |
| 语音模块 | CN-TTS（GBK 合成） | USART2 9600 |
| 电机 | 直流电机 + H 桥 | TIM2 双路 PWM |
| 电位器 | 线性电位器 | ADC1_IN9 |
| LED | 3 路（并联兼容） | PC13/PB12/PA8 |
| 数据输出 | USB 转 TTL | USART3 115200 |

## 5. 技术栈
- 芯片: STM32F103C8T6（Cortex-M3 @72MHz）
- 框架: STM32CubeMX 6.17 + HAL 库
- {"RTOS: FreeRTOS（CMSIS_V1）" if is_rtos else "调度: 裸机主循环 + 非阻塞状态机"}
- 工具链: CMake + arm-none-eabi-gcc（Linux 直接编译）
- 共享纯逻辑: rfid_logic / motor_logic（四版逐字相同，主机单元测试覆盖）

## 6. 构建 / 烧录 / 调试
```bash
# 构建
cmake -B build && cmake --build build     # 或 cmake --preset Debug
# 烧录
st-flash write build/{name}.bin 0x08000000
# 数据输出口监视
screen /dev/ttyUSB0 115200
# 主机单元测试（编译前必跑）
cd ../../stm32/tests && ./run_tests.sh
```
"""
    return {
        "00_阅读指南.md": read,
        "01_项目介绍.md": intro,
        "02_硬件配置.md": hw,
        "03_系统架构.md": arch,
        "04_功能模块.md": mods,
        "05_通信协议.md": proto,
        "06_关键参数表.md": params,
        "07_已知问题与建议.md": issues,
        "08_数据流与控制流.md": dataflow,
        "09_项目结构总览.md": struct,
        "10_代码语义化.md": sem,
        "11_常见问题清单.md": faq,
    }

def esp32_docs(name, is_rtos):
    rt = "RTOS 版" if is_rtos else "裸机版"
    build = "idf.py（ESP-IDF v6.0.2，newlib）"
    hw = f"""# 硬件配置

## 1. 主控芯片
- **芯片**: ESP32-S3（Xtensa LX7 双核，240MHz，512KB SRAM）✅代码确认
- **时钟**: 240MHz 主频（IDF 自动配置）；电机 PWM LEDC 20kHz 10-bit
- **时间基准**: esp_timer_get_time()（64 位 us，应用层截断为 ms）

## 2. 外设资源使用总表
| 外设 | 用途 | 关键参数 | 确认度 |
|---|---|---|---|
| UART1 | U13T 读卡模块 | GPIO10/11，9600→115200 | ✅代码确认 |
| UART2 | CN-TTS 语音模块 | GPIO12/13，9600 | ✅代码确认 |
| LEDC | 电机双通道互补 | GPIO4/5，20kHz，10-bit | ✅代码确认 |
| ADC1 | 电位器 | GPIO1，12-bit，衰减 12dB | ✅代码确认 |
| GPIO | LED×3 | GPIO2/8/9，低电平亮 | ✅代码确认 |
| USB-Serial-JTAG | console 调试输出 | 原生 USB 口 | ✅代码确认 |

## 3. GPIO 引脚分配表（统一在 components/common/pins.h）
| 引脚 | 功能 | 配置 | 确认度 |
|---|---|---|---|
| GPIO1 | 电位器 ADC1_CH0 | 模拟输入 | ✅代码确认 |
| GPIO2 | LED1（低电平亮） | 推挽输出+上拉 | ✅代码确认 |
| GPIO4 | 电机 LEDC CH0 | LEDC 输出 | ✅代码确认 |
| GPIO5 | 电机 LEDC CH1 | LEDC 输出 | ✅代码确认 |
| GPIO8 | LED2 | 推挽输出+上拉 | ✅代码确认 |
| GPIO9 | LED3 | 推挽输出+上拉 | ✅代码确认 |
| GPIO10 | UART1 TX（读卡） | UART 输出 | ✅代码确认 |
| GPIO11 | UART1 RX（读卡） | UART 输入 | ✅代码确认 |
| GPIO12 | UART2 TX（TTS） | UART 输出 | ✅代码确认 |
| GPIO13 | UART2 RX（TTS） | UART 输入 | ✅代码确认 |
| USB-DP/DM | USB-Serial-JTAG console | sdkconfig 配置 | ✅代码确认 |

> 引脚均非 strapping 引脚（GPIO0/3/45/46 已避开）；模块 5V 供电时 RX 引脚（GPIO11/13）为 FT 可直连。
"""
    arch = f"""# 系统架构

## 1. 总体分层
```mermaid
graph LR
    C["components/common: config.h/pins.h"] --> D["components/common: 驱动(card_uart/tts/motor_drv/led/adc/debug)"]
    D --> L["components/common: 纯逻辑(rfid_logic/motor_logic/card_parse)"]
    L --> A["main: 应用状态机"]
    A --> M["app_main 调度"]
```
- 共享组件 components/common 是两版（rtos/baremetal）唯一维护点；main 仅 4 组差异文件（app_main + rfid/motor 任务或进程版），run_tests.sh 漂移检查保证一致
- 纯逻辑层与 STM32 版逐字相同，主机可测

## 2. 启动流程
```mermaid
flowchart TD
    S["上电"] --> H["LED_Init / Motor_Drv_Init / Dbg_Init"]
    H --> C["Card_Uart_Init(含波特率切换 9600→115200)"]
    C --> T["TTS_Init / ADC_Init"]
    T --> M["Motor_Init(电位器采样+状态)"]
    M --> R["RFID_Init(TTS_SetupDefaults+参数)"]
    R --> LOOP["运行时调度"]
"""
    if is_rtos:
        arch += """
## 3. 任务模型（FreeRTOS）
| 任务 | 优先级 | 栈 | 职责 | 文件 |
|---|---|---|---|---|
| RFID_Task | 5 | 4096B | 读卡五态/播报/LED/触发 | main/rfid_task.c |
| Motor_Task | 1 | 2048B | 采样 + 喂 motor_logic + PWM | main/motor_task.c |

- 任务通信：全局 volatile 标志（card_res_flag / motor_trigger_flag）
- FreeRTOS 节拍 100Hz：vTaskDelay(1)=10ms；超时类逻辑用 esp_timer 真实时间差（v0.5 修复漂移）
"""
    else:
        arch += """
## 3. 主循环模型（裸机）
```c
while(1) {
    RFID_Process();   /* 含 Card_Uart_Poll 轮询读卡串口 */
    Motor_Process();
    vTaskDelay(1);    /* 100Hz 节拍下 =10ms，让出 CPU 防看门狗 */
}
```
- 所有延时非阻塞（esp_timer 时间差）；TTS 发送（约 17ms）期间状态机停顿可接受
"""
    arch += """
## 4. 状态机
### 读卡五态（card_res_flag）
```mermaid
stateDiagram-v2
    [*] --> NONE: 上电
    NONE --> EXIST: 读到卡号(200ms 低频探测防失联)
    EXIST --> WAIT: 发读块命令
    WAIT --> EXIST: 20ms 超时重发(2次后放弃)
    WAIT --> RESDATA: 收到读块响应
    RESDATA --> LEDLIGHT: 数据有效→处理
    LEDLIGHT --> NONE: 卡脱离 C 秒后灭灯
```
### 电机状态机（motor_logic，绝对计时）
```mermaid
stateDiagram-v2
    [*] --> IDLE
    IDLE --> RAMPUP: t≥A(2s)
    RAMPUP --> RUN: t≥A+B
    RUN --> SLOW: t∈[E,E+G) 降速F%
    SLOW --> RUN: t≥E+G
    RUN --> STOPPING: 触发停车
    STOPPING --> WAIT: 减速 H=2s
    WAIT --> RAMPUP: 静止 I=10s 后重启
    RUN --> STOP: t≥stop_time 或 1000s
```
"""
    mods = f"""# 功能模块

## 1. 读卡驱动（components/common/card_uart.c|h）
- **职责**: UART1 驱动（环形缓冲，无 ORE 问题）、命令组帧（局部缓冲）、波特率切换、轮询消费
- **对外 API**: Card_Uart_Init/ReadCard/ReadBlock/Poll
- 初始化 API 均 ESP_ERROR_CHECK（配置错误立即暴露）

## 2. 帧解析（components/common/card_parse.c|h，纯逻辑）
- **职责**: U13T 帧解析状态机（长度≤31 防护、仅 0x91 产生事件、LEDLIGHT 卡在场刷新）
- **对外 API**: UartReceiveCommand/Card_Parse_Feed；now_ms 注入，主机可测

## 3. 触发/去重逻辑层（components/common/rfid_logic.c|h，四版共享）
- 与 STM32 版逐字相同（见 stm32 知识库 04_功能模块 §2）

## 4. 电机状态机逻辑层（components/common/motor_logic.c|h，四版共享）
- 与 STM32 版逐字相同（见 stm32 知识库 04_功能模块 §3）

## 5. 电机驱动（components/common/motor_drv.c|h）
- **职责**: LEDC 双通道互补；Motor_Control(0~999)
- 映射：speed=0 → 两路 DUTY_MAX（同电位停）；speed>0 → CH0=0、CH1=speed×1023/999（差分电压）

## 6. TTS（components/common/tts.c|h）
- **职责**: TTS_Init（UART2 9600）、TTS_Send（uart_write_bytes + 等发送完成）、TTS_SetupDefaults（<S>3/<V>6/<I>0 指令集下沉此处）

## 7. LED / ADC / DEBUG（components/common/）
- led.c: 三引脚灯控 + LED_Init（声明在 led.h）+ 边沿调试输出
- adc.c: adc_oneshot 采样（ADC_UNIT/CHANNEL 在 pins.h）
- debug.c: Dbg_Printf → printf（USB-Serial-JTAG console），局部缓冲可重入

## 8. 读卡应用（main/{"rfid_task.c" if is_rtos else "rfid_process.c"}）
- 五态流转 + 播报 + LED 保持 + NONE 态 200ms 低频探测（防询问模式模块失联）
- WAIT 超时用真实时间差（RFID_READ_TIMEOUT_MS），不依赖节拍

## 9. 电机应用（main/{"motor_task.c" if is_rtos else "motor_process.c"}）
- 电位器 20 次采样 → MotorLogic_CalcStopTime → 每拍喂 MotorLogic_Step → Motor_Control

## 10. 入口（main/app_main.c）
- 初始化顺序 + {"xTaskCreate 两任务" if is_rtos else "while(1) 超级循环"}
"""
    proto = f"""# 通信协议

## 1. U13T 读卡协议
{U13T_PROTO.split('### TTS 协议')[0]}
## 2. TTS 语音协议
{U13T_PROTO.split('### TTS 协议')[1].split('### 数据输出口协议')[0]}
## 3. 数据输出口协议
{U13T_PROTO.split('### 数据输出口协议')[1]}
"""
    params = _params_md("""### 硬件映射参数（components/common/pins.h）
| 参数 | 值 | 含义 |
|---|---|---|
| PIN_RFID_TX/RX | GPIO10/11 | 读卡 UART1 |
| PIN_TTS_TX/RX | GPIO12/13 | TTS UART2 |
| PIN_MOTOR_PWM1/2 | GPIO4/5 | 电机 LEDC |
| PIN_ADC_POT | GPIO1 | 电位器 |
| PIN_LED1/2/3 | GPIO2/8/9 | LED |
| MOTOR_PWM_FREQ_HZ | 20000 | 电机 PWM 频率 |
| MOTOR_PWM_RES_BITS | 10 | LEDC 分辨率（0~1023） |
| RFID_UART / TTS_UART | UART_NUM_1 / UART_NUM_2 | 串口实例 |
| ADC_UNIT / ADC_CHANNEL | ADC_UNIT_1 / ADC_CHANNEL_0 | 电位器 ADC |
""")
    issues = f"""# 已知问题与建议

## P0 严重（已修复，保留记录）
### P0-001: FreeRTOS 100Hz 节拍下 WAIT 超时漂移 10 倍（v0.5 修复）
- **文件**: main/rfid_task.c（rfid_process.c）
- **现象**: vTaskDelay(1)=10ms，"1ms 节拍"计数使 20ms 超时实际约 200ms、重发放弃约 400-600ms
- **修复**: 改真实时间差（now_ms() - wait_tick ≥ RFID_READ_TIMEOUT_MS），并修正"1ms 节拍"注释

### P0-002: NONE 态不轮询读卡号，询问模式模块永久失联（v0.5 修复）
- **文件**: main/rfid_task.c（rfid_process.c）
- **现象**: 若模块非自动上报模式，卡重新放入后无响应 → LED/播报/触发全失效
- **修复**: NONE 态每 200ms 发一次 Card_ReadCard 探测

### P0-003: 发送 0x7F 转义分支逻辑错误（v0.5 修复）
- **文件**: components/common/card_uart.c:UartSendCommand
- **现象**: 转义时 i+=1 会跳过输入字节并多读 1 字节（当前命令不含 0x7F 为死代码）
- **修复**: 输入/输出索引分离重写

## P1 重要（已修复，保留记录）
### P1-001: 初始化 API 返回值全忽略（v0.5 修复）
- **修复**: uart/ledc/adc/gpio 初始化统一 ESP_ERROR_CHECK

### P1-002: 两版项目 18 个共享文件复制维护（v0.5 修复）
- **修复**: 抽取 components/common 组件 + run_tests.sh 漂移检查

## P2 遗留（低风险）
| 编号 | 问题 | 影响 | 建议 |
|---|---|---|---|
| P2-001 | RX 0x7F 反转义缺失 | GBK 不含 0x7F，实际不触发 | 解析时双字节合并 |
| P2-002 | speak_en=0 语义 | 仅取消强制播报 | 文档说明 |
| P2-003 | IDLE/RAMPUP 期触发延迟到 RUN 消费 | 上电 6s 内触发先跑后停 | RAMPUP 检查 pending |
| P2-004 | SLOW 窗口重启后二次进入 | 触发 26~31s 时重启后继续降速 | 加持久标志 |
| P2-005 | 绝对计时停车重启后立即 STOP | 设计既定 | 确认规则 |
| P2-006 | 中文路径依赖 newlib 配置 | 换 ASCII 路径可移除 CONFIG_LIBC_NEWLIB=y | 文档已注明 |
"""
    dataflow = f"""# 数据流与控制流

## 1. 读卡数据流
```mermaid
flowchart TD
    U["UART1 环形缓冲"] --> P["Card_Uart_Poll(每圈轮询)"]
    P --> F["Card_Parse_Feed 帧解析"]
    F --> FLAG["card_res_flag 置位"]
    FLAG --> SM["RFID 状态机"]
    SM --> D["chinese_data[16]"]
    D --> LG["RfidLogic_Process"]
    LG -->|EV_SPEAK| TTS["TTS_Send(UART2) + UpdateSpeak"]
    LG -->|EV_TRIGGER_STOP| TF["motor_trigger_flag=1"]
    LG -->|EV_NONE| SKIP["去重拦截"]
```
## 2. 电机控制流
```mermaid
flowchart TD
    ADC["adc_oneshot 采样×20"] --> CT["MotorLogic_CalcStopTime"]
    CT --> S["MotorLogic_Step"]
    TF["motor_trigger_flag"] --> S
    S --> MC["Motor_Control(speed)"]
    MC --> PWM["LEDC 双通道 GPIO4/5"]
```
## 3. 触发停车完整链路
读卡 → rfid_logic 命中规则 → EV_TRIGGER_STOP → motor_trigger_flag → MotorLogic_Step 消费 → STOPPING(2s)→WAIT(10s)→RAMPUP→RUN
## 4. LED 控制流
LEDLIGHT：LED_Sta(1) → 800ms 后每 10ms ReadCard → 回调刷新 rfid_last_card_tick → 脱离 C=3s 灭灯；NONE 态 200ms 探测读卡号
"""
    struct = f"""# 项目结构总览

## 1. 目录结构
```
esp32/
├── components/common/            # ★ 共享业务组件（两版唯一维护点）
│   ├── CMakeLists.txt            # idf_component_register(REQUIRES esp_driver_* ...)
│   ├── config.h                  # 全部可调参数（触发词表/时序）
│   ├── pins.h                    # GPIO + UART/ADC/LEDC 实例映射
│   ├── rfid_logic.c|h            # 触发/去重纯逻辑（四版共享）
│   ├── motor_logic.c|h           # 电机状态机纯逻辑（四版共享）
│   ├── card_parse.c|h            # 帧解析纯逻辑
│   ├── card_uart.c|h             # UART1 驱动
│   ├── tts.c|h                   # TTS 发送 + 默认设置
│   ├── motor_drv.c|h             # LEDC 电机驱动
│   └── led.c|h adc.c|h debug.c|h
└── {name}/                        # 本工程（仅应用层差异文件）
    ├── CMakeLists.txt            # EXTRA_COMPONENT_DIRS → ../components（必须在 project() 前）
    ├── sdkconfig.defaults        # USB-Serial-JTAG + CONFIG_LIBC_NEWLIB=y（中文路径绕行）
    └── main/
        ├── CMakeLists.txt        # SRCS：app_main + {"rfid_task + motor_task" if is_rtos else "rfid_process + motor_process"}，REQUIRES common
        ├── app_main.c
        ├── {"rfid_task.c|h / motor_task.c|h" if is_rtos else "rfid_process.c|h / motor_process.c|h"}
```
## 2. 文件地图
| 文件 | 职责 | 关键函数 |
|---|---|---|
| app_main.c | 初始化 + 调度 | app_main |
| card_uart.c | 读卡 UART 驱动 | Card_Uart_Init/Poll |
| card_parse.c | 帧解析 | UartReceiveCommand/Card_Parse_Feed |
| rfid_logic.c | 触发/去重 | RfidLogic_Process |
| motor_logic.c | 电机状态机 | MotorLogic_Step |
| tts.c | 语音 | TTS_Send/SetupDefaults |
| motor_drv.c | LEDC | Motor_Control |
| led.c/adc.c/debug.c | 灯/电位器/输出 | LED_Sta/Get_ADC_Value/Dbg_Printf |
| {"rfid_task.c" if is_rtos else "rfid_process.c"} | 读卡状态机 | {"RFID_Task" if is_rtos else "RFID_Process"} |
| {"motor_task.c" if is_rtos else "motor_process.c"} | 电机应用 | {"Motor_Task" if is_rtos else "Motor_Process"} |

## 3. 关键依赖与注意事项
- **共享组件**：components/common 是两版唯一维护点；run_tests.sh 漂移检查保证 main 仅 4 组差异文件
- **中文路径**：编译依赖 sdkconfig.defaults 的 CONFIG_LIBC_NEWLIB=y（勿删）；换 ASCII 路径可移除
- **构建**：EXTRA_COMPONENT_DIRS 必须在 project() 前设置；每次编译前 source 激活脚本
- **测试**：esp32/tests/run_tests.sh（rfid_logic 32 + card_parse 14 + motor_logic 30 + 漂移检查）
"""
    sem = f"""# 代码语义化

> 核心函数卡片（优先级 1 文件全部导出函数）

## components/common 纯逻辑（与 STM32 版逐字相同，语义见 stm32 知识库 10_代码语义化）
- rfid_logic.c: RfidLogic_Init/TriggerMatch/IsDup/UpdateSpeak/Process/RuleCount
- motor_logic.c: MotorLogic_Init/Step/IsInStopSequence/CalcStopTime/StateName
- card_parse.c: UartReceiveCommand/Card_Parse_Feed（帧解析 + 状态标志更新）

## card_uart.c（读卡驱动）
| 函数 | 语义 |
|---|---|
| Card_Uart_Init | UART1 安装（环形缓冲 256B）→ 9600 发 0x2C → 50ms → uart_set_baudrate(115200)；全部 ESP_ERROR_CHECK |
| Card_ReadCard / Card_ReadBlock | 局部缓冲组帧（0x7F 头 + 异或校验 + 转义）→ uart_write_bytes + wait_tx_done |
| Card_Uart_Poll | uart_read_bytes(0 超时) 非阻塞取字节 → Card_Parse_Feed(buf[i], now_ms) |

## tts.c
| 函数 | 语义 |
|---|---|
| TTS_Init | UART2 安装（9600 8N1） |
| TTS_SetupDefaults | 发送 <S>3/<V>6/<I>0（语速/音量/断电保存） |
| TTS_Send | 0 结尾字符串逐段 uart_write_bytes + wait_tx_done |

## motor_drv.c
| 函数 | 语义 |
|---|---|
| Motor_Drv_Init | LEDC 定时器 20kHz 10-bit + 双通道配置（初始 DUTY_MAX） |
| Motor_Control | speed=0 → 双路 DUTY_MAX 同电位停；>0 → CH0=0、CH1=speed×1023/999 |

## {"rfid_task.c（读卡任务）" if is_rtos else "rfid_process.c（读卡状态机）"}
- {"RFID_Task" if is_rtos else "RFID_Init + RFID_Process"}：五态流转；WAIT 真实时间差超时（重发 2 次后放弃）；NONE 200ms 探测；LEDLIGHT 800ms 后 10ms 轮询；数据经 RfidLogic_Process 决策
- RFID_HandleCardData：事件执行（置 motor_trigger_flag / TTS_Send + UpdateSpeak）
- RFID_Speak / BufClear：播报与缓冲清理（上限 RFID_BLOCK_SIZE）

## {"motor_task.c（电机任务）" if is_rtos else "motor_process.c（电机状态机）"}
- {"Motor_Task" if is_rtos else "Motor_Init + Motor_Process"}：20 次采样 → CalcStopTime → 每拍 MotorLogic_Step（传入 trigger 后清零标志）→ Motor_Control；状态变化边沿输出
- Motor_IsInStopSequence：转发 MotorLogic_IsInStopSequence
"""
    faq = _faq_md()
    read = f"""# 阅读指南

## 项目一句话
"{name}"：ESP32-S3 {rt}，实现 U13T 读卡 + CN-TTS 语音播报 + 电机控制（多触发词停车/定时降速/电位器停止）+ LED + USB-Serial-JTAG 调试输出。

## 文档地图
| 文档 | 内容 | 适合谁 |
|---|---|---|
| 01_项目介绍 | 功能清单/硬件组成/技术栈 | 所有人 |
| 02_硬件配置 | 芯片/外设/引脚 | 硬件调试 |
| 03_系统架构 | 分层/启动流程/状态机 | 架构理解 |
| 04_功能模块 | 10 个业务模块 | 功能开发 |
| 05_通信协议 | U13T 帧协议/TTS/输出协议 | 联调 |
| 06_关键参数表 | config.h/pins.h 全参数 | 调参 |
| 07_已知问题与建议 | P0-P2 问题清单 | 维护 |
| 08_数据流与控制流 | 链路图 | 排障 |
| 09_项目结构总览 | 目录树/文件地图 | 新人 |
| 10_代码语义化 | 核心函数卡片 | 改代码前 |
| 11_常见问题清单 | 20 个 Q&A | 快速求助 |

## 快速上手路径
1. 新人: 01 → 09 → 03
2. 调参: 06 → 01
3. 排障: 11 → 08 → 07
4. 开发: 04 → 10

## 关键提醒
- 共享组件在 ../components/common（两版唯一维护点，run_tests.sh 有漂移检查）
- 中文路径编译依赖 sdkconfig.defaults 的 CONFIG_LIBC_NEWLIB=y（勿删）
- 每次编译前先 `source <HOME>/.espressif/tools/activate_idf_v6.0.2.sh`
"""
    intro = f"""# 项目介绍

## 1. 项目概述
{name} 是 STM32 复刻工程的 ESP32-S3 移植版（{rt}），功能与 STM32 版完全一致：U13T 读卡 + CN-TTS 播报 + 电机控制 + LED + 调试输出。共享纯逻辑层（rfid_logic/motor_logic/card_parse）与 STM32 版逐字相同。

## 2. 功能清单
{FUNC_SPEC}

## 3. 触发词行为矩阵（功能 6 详解）
{TRIGGER_MATRIX}

> 停车序列：触发 → STOPPING（H=2s 线性减速）→ WAIT（I=10s 静止）→ RAMPUP（重启缓启动）→ RUN；共 H+I=12s。

## 4. 硬件组成
| 部件 | 型号/规格 | 接口 |
|---|---|---|
| 主控 | ESP32-S3（240MHz，512KB SRAM） | — |
| 读卡模块 | U13T（13.56MHz） | UART1 GPIO10/11 |
| 语音模块 | CN-TTS | UART2 GPIO12/13 |
| 电机 | 直流电机 + H 桥 | LEDC GPIO4/5 |
| 电位器 | 线性电位器 | ADC1_CH0 GPIO1 |
| LED | 3 路 | GPIO2/8/9 |
| 调试输出 | USB-Serial-JTAG | 原生 USB |

## 5. 技术栈
- 芯片: ESP32-S3；框架: ESP-IDF v6.0.2（newlib）
- {"调度: FreeRTOS 双任务（RFID_Task pri=5 / Motor_Task pri=1）" if is_rtos else "调度: app_main 单任务超级循环"}
- 共享组件: components/common（config/pins/驱动/纯逻辑）
- 测试: esp32/tests（rfid_logic + card_parse + motor_logic + 漂移检查）

## 6. 构建 / 烧录 / 调试
```bash
# 激活环境（每次编译前）
source <HOME>/.espressif/tools/activate_idf_v6.0.2.sh
# 构建
cd esp32/{name} && idf.py build
# 烧录 + 监视（USB-Serial-JTAG）
idf.py -p /dev/ttyACM0 flash monitor
# 主机单元测试（编译前必跑）
cd ../tests && ./run_tests.sh
```
"""
    return {
        "00_阅读指南.md": read,
        "01_项目介绍.md": intro,
        "02_硬件配置.md": hw,
        "03_系统架构.md": arch,
        "04_功能模块.md": mods,
        "05_通信协议.md": proto,
        "06_关键参数表.md": params,
        "07_已知问题与建议.md": issues,
        "08_数据流与控制流.md": dataflow,
        "09_项目结构总览.md": struct,
        "10_代码语义化.md": sem,
        "11_常见问题清单.md": faq,
    }

def main():
    gen_doc("stm32_rfid_tts_rtos", stm32_docs("stm32_rfid_tts_rtos", True))
    gen_doc("stm32_rfid_tts_baremetal", stm32_docs("stm32_rfid_tts_baremetal", False))
    gen_doc("esp32_tts_rtos", esp32_docs("esp32_tts_rtos", True))
    gen_doc("esp32_tts_baremetal", esp32_docs("esp32_tts_baremetal", False))
    print("12 文档 × 4 项目生成完成")

if __name__ == "__main__":
    main()
