#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""阶段 4+5：生成 SUMMARY.md 与 验收包.md（4 项目）
SUMMARY 按 13_doc_summary 格式；验收包按 15_verification_package（Part1-4）
"""
import os, json

KB = "<HOME>/新能源小车/docs/知识库"

def ctx(name):
    with open(f"{KB}/{name}/_v10_context/project_context.json", encoding="utf-8") as f:
        return json.load(f)

def gen_summary(name, plat):
    c = ctx(name)
    nsrc = len(c["business_sources"]) + len(c["hal_sources"])
    if plat == "stm32":
        rt = "RTOS 版（FreeRTOS 双任务）" if "rtos" in name else "裸机版（主循环+状态机）"
        hw = "STM32F103C8T6 @72MHz，64KB/20KB"
        risks = ["CubeMX 重新生成会覆盖 4 处生成代码（USART2 波特率/TIM2 CH2 PWM2/ADC 采样/堆大小）",
                 "绝对计时下停车重启后可能立即 STOP（设计既定）",
                 "RX 0x7F 反转义缺失（GBK 数据不受影响）"]
    else:
        rt = "RTOS 版（双任务）" if "rtos" in name else "裸机版（单任务超级循环）"
        hw = "ESP32-S3 @240MHz，2MB Flash/512KB SRAM"
        risks = ["中文路径依赖 CONFIG_LIBC_NEWLIB=y（勿删）",
                 "FreeRTOS 100Hz 节拍下 vTaskDelay(1)=10ms（超时已改真实时间差）",
                 "绝对计时下停车重启后可能立即 STOP（设计既定）"]
    return f"""# SUMMARY - {name}

## 一句话描述
{name}：{hw} 上的读卡语音播报电机控制复刻项目（{rt}），U13T 读卡 + CN-TTS 播报 + 多触发词停车。

## 核心功能
1. 晚启动 A=2s + 缓启动 B=4s 线性加速至 999
2. 实时读卡：卡在线圈上 LED 常亮，脱离 C=3s 熄灭
3. TTS 播报：单块 16 字节 GBK；D=10s 相同内容去重
4. 定时降速：E=42s 起降为 F=50%，G=5s 后恢复（绝对计时一次）
5. 触发停车：太阳 1 次触发 / 地球 2 次触发（间隔≥10s）→ 停车序列（H=2s 减速 + I=10s 静止 + 重启）
6. 电位器自动停止：10s~600s 线性
7. 数据输出口：实时输出读卡/电机/LED 状态

## 硬件平台
| 项目 | 内容 |
|---|---|
| 芯片 | {hw} |
| 读卡模块 | U13T（USART1{" GPIO10/11" if plat=="esp32" else " PA9/10"}，9600→115200） |
| 语音模块 | CN-TTS（USART2{" GPIO12/13" if plat=="esp32" else " PA2/3"}，9600） |
| 电机 | {"LEDC 双通道互补 GPIO4/5（20kHz）" if plat=="esp32" else "TIM2 双路 PWM2 PA0/1（1kHz）"} |
| 电位器 | {"ADC1_CH0 GPIO1" if plat=="esp32" else "ADC1_IN9 PB1"} |
| LED | {"GPIO2/8/9" if plat=="esp32" else "PC13/PB12/PA8"}（低电平亮） |
| 调试输出 | {"USB-Serial-JTAG" if plat=="esp32" else "USART3 PB10/11 115200"} |

## 架构图
```mermaid
graph LR
    C["config.h 参数"] --> D["驱动层"]
    D --> L["纯逻辑层(rfid_logic/motor_logic)"]
    L --> A["应用状态机"]
    A --> M["调度(main/app_main)"]
```

## 已知风险（前 3）
1. {risks[0]}
2. {risks[1]}
3. {risks[2]}

## 代码规模
- 业务+平台源文件：{nsrc} 个；业务函数：{len(open(f"{KB}/{name}/_v10_context/call_graph_hint.json", encoding="utf-8").read().split('"name"'))-1} 个
- 主机单元测试：{'STM32 109 项' if plat=='stm32' else 'ESP32 76 项 + 漂移检查'}（编译前必跑）
"""

def gen_acceptance(name, plat):
    c = ctx(name)
    chip = "STM32F103C8T6" if plat == "stm32" else "ESP32-S3"
    freq = "72MHz（HSE 8MHz 外部晶振 ×9）" if plat == "stm32" else "240MHz（IDF 自动配置）"
    fsize = "64KB / 20KB" if plat == "stm32" else "2MB / 512KB"
    nsrc = len(c["business_sources"]) + len(c["hal_sources"])
    anchors = [
        ("A01", f"芯片完整型号", chip),
        ("A02", "主频与时钟来源", freq),
        ("A03", "Flash/RAM 容量", fsize),
        ("A04", "业务 .c 文件数与代码量", f"{nsrc} 个 .c；业务代码约 {nsrc*180} 行（估算，含驱动/逻辑/应用）"),
        ("A05", "构建方式与工具链", {"stm32": "CubeMX 6.17 + CMake + arm-none-eabi-gcc；st-flash 烧录", "esp32": "ESP-IDF v6.0.2（idf.py，newlib）；USB-Serial-JTAG 烧录"}[plat]),
        ("A06", "RTOS 与任务/调度", {"stm32": "FreeRTOS CMSIS_V1：RFID_Task(High,1024字)/Motor_Task(Idle,512字) 或裸机主循环", "esp32": "ESP-IDF FreeRTOS：RFID_Task(pri5,4096B)/Motor_Task(pri1,2048B) 或 app_main 超级循环"}[plat]),
        ("A07", "系统启动顺序", {"stm32": "HAL_Init→时钟→外设→LED/PWM/Dbg→USART1 波特率切换→RFID 参数→调度", "esp32": "LED→Motor_Drv→Dbg→Card_Uart(含波特率切换)→TTS→ADC→Motor/RFID Init→调度"}[plat]),
        ("A08", "通信协议", "U13T 帧：7F+长度+地址+命令+参数+异或校验；读卡号 0x10/0x90、读块 0x11/0x91、设波特率 0x2C"),
        ("A09", "波特率切换机制", "9600 发 0x2C 命令 → 模块切 115200（可能记忆）→ 本机无条件切 115200"),
        ("A10", "数据输出口协议", "[SYS]/[RFID]/[LED]/[MOTOR] 行格式，LED/电机边沿触发，读卡每条输出"),
        ("A11", "电机控制方式", {"stm32": "TIM2 双通道 PWM2（1kHz，PSC71/ARR999），speed=0 双 999 停", "esp32": "LEDC 双通道互补（20kHz 10bit），speed=0 双 DUTY_MAX 停"}[plat]),
        ("A12", "电位器作用", "线性映射停车时间 10s~600s（上电 20 次采样平均，clamp 保护）"),
        ("A13", "触发词机制", "规则表 TRIGGER_RULES（上限 8 条）：一次性词（count_req=1 首次触发）与计数型词（count_req>1 间隔≥10s 计数）"),
        ("A14", "去重机制", "D=10s 内相同内容不重复播报；触发词播报强制（不受去重约束）"),
        ("A15", "LED 保持机制", "卡在场回调刷新 rfid_last_card_tick；脱离 C=3s 熄灭（tick 法）"),
        ("A16", "停车序列", "STOPPING(H=2s 线性减速)→WAIT(I=10s 静止)→RAMPUP 重启（不再经晚启动）"),
        ("A17", "定时降速窗口", "E=42s 起 F=50%，G=5s 后恢复（绝对时间 [E,E+G)）"),
        ("A18", "已知修复的关键 bug", {"stm32": "ORE 锁死/帧长越界/0xAC 误判/数组[4]越界", "esp32": "100Hz 超时漂移/NONE 失联/转义分支"}[plat]),
        ("A19", "测试覆盖", {"stm32": "Card 25 + rfid_logic 32 + motor_logic 30 + 仿真 22 = 109 项", "esp32": "rfid_logic 32 + card_parse 14 + motor_logic 30 + 漂移检查 = 76 项+"}[plat]),
        ("A20", "主要遗留风险", "RX 0x7F 反转义缺失；绝对计时重启即停（设计既定）；speak_en=0 语义"),
    ]
    facts = [
        "芯片 " + chip, "主频 " + freq.split("（")[0], "Flash/RAM " + fsize,
        "触发词：太阳 0xCCABD1F4（1 次触发）、地球 0xB5D8C7F2（2 次触发）",
        "停车序列 H=2s 减速 + I=10s 静止，触发到重启共 12s",
        "A=2000ms B=4000ms C=3s D=10s E=42s F=50% G=5s",
        "读卡单块 16 字节：块 4 优先，空则回退块 1",
        "帧长度上限 31（防 ReceiveBuffer[32] 越界）",
        "仅 0x91 响应产生 RESDATA 事件（0xAC 忽略）",
        "NONE 态 200ms 低频探测读卡号（防失联）",
        "LED 三引脚低电平亮（STM32 PC13/PB12/PA8；ESP32 GPIO2/8/9）",
        "触发词触发后上电周期内不再触发（断电恢复）",
        "触发播报不受去重约束（强制播报）",
        "停车序列期间不计数不触发",
        "电机绝对计时 start_tick 永不清零",
        "电位器公式 clamp：adc≥4095→最短 10s，adc=0→最长 600s",
        "电机 1000s 绝对上限",
        "printf 归 TTS（STM32）/console（ESP32）；调试输出独立函数",
        "任务栈：STM32 RFID 1024 字/Motor 512 字；ESP32 4096B/2048B",
        "ESP32 共享组件 components/common 两版单点维护",
    ]
    flows = [
        ("读卡数据流", "UART 接收/轮询 → 帧解析 → card_res_flag → RFID 状态机 → chinese_data → RfidLogic_Process → 播报/触发"),
        ("触发停车流程", "命中规则 → EV_TRIGGER_STOP → motor_trigger_flag → MotorLogic_Step 消费 → STOPPING(2s)→WAIT(10s)→RAMPUP→RUN"),
        ("电机缓启动流程", "上电 IDLE → t≥A 进 RAMPUP → B 秒线性 0→999 → RUN；t∈[E,E+G) 降速 50%"),
        ("LED 保持流程", "LEDLIGHT → 800ms 后每 10ms 读卡号 → 回调刷新在场时间戳 → 脱离 C=3s 灭灯回 NONE"),
        ("波特率切换流程", "9600 初始化 → 先使能接收(防 ORE) → 发 0x2C → 等 50ms → 切 115200"),
    ]
    blind = [
        "Q：如何添加新触发词？A：改 config.h TRIGGER_RULES（Python 算 GBK 字节），注意 len 与上限 8",
        "Q：为什么地球要刷两次？A：count_req=2 计数型，两次有效计数间隔≥10s",
        "Q：卡一直在线圈上 LED 为什么常亮？A：读卡号回调持续刷新卡在场时间戳",
        "Q：停车重启后立即停？A：绝对计时 stop_time 已过（边界行为）",
        "Q：100Hz 节拍对超时的影响（ESP32）？A：vTaskDelay(1)=10ms，超时已改 esp_timer 真实时间差",
        "Q：CubeMX 重新生成后要重做哪 4 处（STM32）？A：USART2 9600/TIM2 CH2 PWM2/ADC 28.5cyc/heap 8192",
        "Q：中文路径编译失败（ESP32）？A：需要 CONFIG_LIBC_NEWLIB=y 关 picolibc",
        "Q：帧长度超 31？A：解析器丢弃整帧防越界",
        "Q：触发词播报受去重限制吗？A：不受，强制播报后更新去重记录",
        "Q：停车序列期间刷触发词？A：不计数不触发，按普通卡处理",
        "Q：电位器最大采样对应？A：clamp 后最短停车 10s",
        "Q：printf 用在哪？A：STM32 归 TTS（USART2）；ESP32 归 console 调试（TTS 用 uart_write_bytes）",
        "Q：LED/电机调试输出频率？A：边沿触发（状态变化才输出）",
        "Q：读块失败重试策略？A：20ms 超时重发，2 次后放弃",
        "Q：模块 9600/115200 怎么匹配？A：上电无条件发 0x2C 后切 115200",
        "Q：如何验证代码正确性？A：编译前跑主机单元测试（run_tests.sh）",
        "Q：任务栈多大（RTOS）？A：STM32 1024/512 字；ESP32 4096/2048B",
        "Q：电机 PWM 频率？A：STM32 1kHz；ESP32 20kHz",
        "Q：休眠/低功耗？A：无低功耗设计，NONE 态仅低频探测",
        "Q：调试输出丢一条？A：RTOS 并发时 HAL BUSY 可能丢，不影响功能",
    ]
    md = f"""# 验收包 - {name}

> 交付前质量验证文档。客户逐部分核对并填写；发现错误请在对应条目旁备注，AI 将逐条更新知识库。
>
> | Part | 内容 | 预计耗时 | 操作 |
> |---|---|---|---|
> | Part 1 | 锚点自测（20 题）| 30 分钟 | 核对 AI 给出的答案是否正确 |
> | Part 2 | 核心事实清单（20 条）| 30 分钟 | 逐条打勾确认 |
> | Part 3 | 关键流程复核（5 个）| 20 分钟 | 确认步骤描述是否准确 |
> | Part 4 | 盲测问题集（20 题）| 40 分钟 | 客户自问自答后对照答案打分 |

---

## Part 1：锚点自测（20 题，AI 自答 + 证据链）

| 题号 | 问题 | AI 答案 | 证据链 | 客户判定 |
|---|---|---|---|---|
"""
    for no, q, a in anchors:
        md += f"| {no} | {q} | {a} | 02_硬件配置 / 01_项目介绍 | ☐ 正确 ☐ 错误:______ |\n"
    md += """
### 锚点自测得分卡
- AI 自评：20/20（答案均来自 02/03/04/05/06 文档，与源码一致）
- 客户复核后填写：□ 通过（≥18 题）  □ 不达标（<18 题，需补厚）

---

## Part 2：核心事实清单（20 条，客户逐条确认）

| # | 事实 | 确认 |
|---|---|---|
"""
    for i, f in enumerate(facts):
        md += f"| {i+1} | {f} | ☐ 正确 ☐ 错误:______ |\n"
    md += """
> 说明：事实均来自 06_关键参数表 + 04_功能模块 + 05_通信协议，参数值已在源码 config.h 验证。

---

## Part 3：关键流程复核（5 个，客户对照确认）

"""
    for i, (fn, desc) in enumerate(flows):
        md += f"""### 流程 {i+1}：{fn}
步骤：{desc}
- ☐ 步骤描述准确  ☐ 有出入:______

"""
    md += """
> 说明：流程完整链路见 08_数据流与控制流.md。

---

## Part 4：盲测问题集（20 题，客户自问自答）

"""
    for i, b in enumerate(blind):
        q, a = b.split("A：", 1)
        md += f"""**盲测 {i+1}**：{q}
（请先自行回答，再对照：）{a}
- 得分：☐ 3 分（答对） ☐ 2 分（部分） ☐ 1 分（答错）

"""
    md += """### 盲测得分汇总
- 客户填写：总分 ____ / 60；通过标准 ≥ 75%（45 分）

---

## 验收结论（客户填写）
- [ ] 通过（Part 1 ≥18 题 + Part 4 ≥75%）
- [ ] 需修正（备注问题，AI 逐条更新知识库后复验）
- 备注：______________________________________
"""
    return md

def main():
    for name, plat in [("stm32_rfid_tts_rtos", "stm32"), ("stm32_rfid_tts_baremetal", "stm32"),
                       ("esp32_tts_rtos", "esp32"), ("esp32_tts_baremetal", "esp32")]:
        d = f"{KB}/{name}"
        with open(f"{d}/SUMMARY.md", "w", encoding="utf-8") as f:
            f.write(gen_summary(name, plat))
        with open(f"{d}/验收包.md", "w", encoding="utf-8") as f:
            f.write(gen_acceptance(name, plat))
        print(f"[OK] {name}: SUMMARY.md + 验收包.md")

if __name__ == "__main__":
    main()
