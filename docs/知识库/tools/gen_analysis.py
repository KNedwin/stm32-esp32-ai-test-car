#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""第 2 层：生成 _analysis/ 13 个中间分析文件（导航索引 + 核心数据）
基于 v0.5 源码状态（关键数值已 grep 验证）
"""
import json, os

KB = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))  # docs/知识库=脚本上两级, 自动定位

STM32_COMMON = {
    "device": "STM32F103C8T6",
    "core": "Cortex-M3 @72MHz",
    "clock": """HSE 8MHz --x9--> SYSCLK 72MHz; AHB /1=72M; APB1 /2=36M(TIM2 x2=72M); APB2 /1=72M; ADC /6=12M""",
    "flash": "0x08000000, 64KB", "ram": "0x20000000, 20KB",
}

def proj_ctx(name):
    with open(f"{KB}/{name}/_v10_context/project_context.json", encoding="utf-8") as f:
        return json.load(f)

def funcs_of(name):
    with open(f"{KB}/{name}/_v10_context/call_graph_hint.json", encoding="utf-8") as f:
        return json.load(f)["functions"]

def w(name, fname, content):
    d = f"{KB}/{name}/_analysis"
    os.makedirs(d, exist_ok=True)
    with open(f"{d}/{fname}", "w", encoding="utf-8") as f:
        f.write(content)

def gen_stm32(name, is_rtos):
    c = proj_ctx(name)
    funcs = funcs_of(name)
    rt = "RTOS 版（FreeRTOS CMSIS_V1）" if is_rtos else "裸机版（无 RTOS，主循环+状态机）"
    t1 = "TIM1（FreeRTOS 占用 SysTick）" if is_rtos else "SysTick"

    w(name, "project_overview.md", f"""# 项目概览（导航）
- 芯片: **STM32F103C8T6**（Cortex-M3，64KB Flash / 20KB RAM）
- 版本: {rt}；构建: CubeMX 6.17 生成 + CMake/GCC（arm-none-eabi 14.2.1）
- 功能: 读卡(U13T) + TTS 播报 + 电机控制（多触发词停车/降速/电位器停止）+ LED + USART3 调试口
- 业务源文件: {len(c['business_sources'])} 个；头文件: {len(c['header_files'])} 个；业务函数: {len(funcs)}
- 业务文件清单: {', '.join(s['relative_path'] for s in c['business_sources'])}
- 有效宏: {', '.join(c['active_macros'])}
""")

    w(name, "clock_tree.md", f"""# 时钟树
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
- 时钟源: HSE 8MHz 晶振；PLL MUL=9（sources/main.c:SystemClock_Config）
- 时基: HAL {t1}；FreeRTOS 节拍（RTOS 版）: SysTick 1000Hz（configTICK_RATE_HZ 默认）
- 定时器用途: TIM2=PWM 电机 1kHz（PSC=71, ARR=999）；TIM1（RTOS 版）=HAL 时基 1kHz
""")

    w(name, "gpio_and_pins.md", f"""# GPIO 与引脚（导航）
| 引脚 | 功能 | 证据 |
|---|---|---|
| PA0 | TIM2_CH1 电机 PWM | sources/tim.c:HAL_TIM_MspPostInit（AF_PP） |
| PA1 | TIM2_CH2 电机 PWM | sources/tim.c:HAL_TIM_MspPostInit |
| PA2 | USART2_TX TTS | sources/usart.c:HAL_UART_MspInit |
| PA3 | USART2_RX TTS | sources/usart.c:HAL_UART_MspInit |
| PA8 | LED（低电平亮） | sources/gpio.c:MX_GPIO_Init（SET 上拉） |
| PA9 | USART1_TX 读卡 | sources/usart.c:HAL_UART_MspInit |
| PA10 | USART1_RX 读卡 | sources/usart.c:HAL_UART_MspInit |
| PA13/14 | SWD | sources/gpio.c（Serial Wire） |
| PB1 | ADC1_IN9 电位器 | sources/adc.c:HAL_ADC_MspInit（模拟输入） |
| PB10 | USART3_TX 调试输出 | sources/usart.c:HAL_UART_MspInit |
| PB11 | USART3_RX 调试输出 | sources/usart.c:HAL_UART_MspInit |
| PB12 | LED | sources/gpio.c:MX_GPIO_Init |
| PC13 | LED | sources/gpio.c:MX_GPIO_Init |
| PD0/1 | HSE 8MHz 晶振 | sources/main.c:RCC |
""")

    w(name, "peripheral_config.md", f"""# 外设配置（导航）
| 外设 | 配置 | 关键参数 | 证据 |
|---|---|---|---|
| USART1 | 读卡 9600→115200 | 8N1；init 后 SetBound115200 切 115200 | sources/usart.c:USART1_Init 2；hardware/rfid_card/Card.c |
| USART2 | TTS 9600 | 8N1；__io_putchar 重定向 | sources/usart.c；hardware/USART/BSP_USART.c |
| USART3 | 调试输出 115200 | 8N1；Dbg_Printf | sources/usart.c；hardware/DEBUG/Debug.c |
| TIM2 | 电机 PWM 1kHz | PSC=71 ARR=999 PWM2 双通道 OCFast ENABLE | sources/tim.c:MX_TIM2_Init |
| ADC1 | 电位器 IN9 | 28.5 cycles 连续转换 | sources/adc.c:MX_ADC1_Init |
| GPIO | LED×3 | 推挽+上拉 初始高 | sources/gpio.c |
""")

    w(name, "nvic_priorities.md", f"""# NVIC 优先级（组 4：4 位抢占）
| 中断 | 抢占优先级 | 说明 |
|---|---|---|
| USART1 | 5 | 读卡接收 |
| USART2 | 5 | TTS（RX 未用） |
| {'TIM1_UP | 15 | HAL 时基（RTOS 版）' if is_rtos else 'SysTick | 15(默认) | HAL 时基' } |
""")

    w(name, "system_init_and_tasks.md", f"""# 启动顺序与任务
## 启动顺序（main）
HAL_Init → SystemClock_Config → MX_GPIO/ADC1/TIM2/USART1/USART2/USART3 → LED_Sta(0)/PWM_Init/Motor_Control(0)/Dbg_Init → {'MX_FREERTOS_Init + osKernelStart' if is_rtos else 'Motor_Init + RFID_Init（TTS 设置+参数）→ while(1){RFID_Process; Motor_Process}'}
- USART1 init 内完成波特率切换（先使能接收防 ORE → SetBound115200 → 重 init 115200）
## 任务（RTOS 版）
| 任务 | 优先级 | 栈 | 职责 |
|---|---|---|---|
| RFID_Task | High(2) | 1024 字 | 读卡/播报/LED/触发（rfid_task.c） |
| Motor_Control_Task | Idle(-3) | 512 字 | 电机状态机（motor_control_task.c，喂 motor_logic） |
| defaultTask | Normal | 128 字 | 空转（CubeMX 遗留） |
""")

    w(name, "memory_layout.md", f"""# 内存布局
- Flash: 0x08000000，64KB（.ld: STM32F103XX_FLASH.ld，链接产物约 34KB（RTOS）/28KB（裸机））
- RAM: 0x20000000，20KB；RTOS 版 bss≈11.7KB（含 FreeRTOS 堆 8192B），裸机版 bss≈2.5KB
- 栈: main 0x400；FreeRTOS 堆 configTOTAL_HEAP_SIZE=8192（RTOS 版）
""")

    modules = """| 模块 | 文件 | 职责 | 核心函数 |
|---|---|---|---|
| 读卡协议 | hardware/rfid_card/Card.c|h | U13T 帧收发/解析、波特率切换、接收回调 | SetBound115200/ReadCard/ReadBlock/UartReceiveCommand/HAL_UART_RxCpltCallback/HAL_UART_ErrorCallback |
| 触发/去重逻辑 | Task/rfid_logic.c|h | 触发词匹配/计数/去重决策（纯逻辑，四版共享） | RfidLogic_Process/TriggerMatch/IsDup/UpdateSpeak |
| 电机状态机 | Task/motor_logic.c|h | 电机时序纯逻辑（四版共享） | MotorLogic_Step/Init/CalcStopTime/IsInStopSequence |
| 读卡状态机 | Task/{'rfid_task.c' if is_rtos else 'rfid_process.c'}|h | 五态读卡流程+播报+LED | RFID_Task/RFID_Process |
| 电机应用 | Task/{'motor_control_task.c' if is_rtos else 'motor_process.c'}|h | 采样+喂 motor_logic+输出 PWM | Motor_Control_Task/Motor_Process |
| TTS 发送 | hardware/USART/BSP_USART.c|h | printf 重定向(__io_putchar→USART2)、字符串发送 | Usartx_SendString |
| 电机驱动 | hardware/pwm/PWM.c|h | Motor_Control(0~999) 双路 PWM2 | Motor_Control/PWM_Init |
| LED | hardware/LED/led.c|h | 三引脚灯控（边沿调试输出） | LED_Sta |
| 电位器 | hardware/ADC/BSP_ADC.c|h | ADC 采样 | Get_ADC_Value |
| 调试输出 | hardware/DEBUG/Debug.c|h | USART3 数据口 | Dbg_Init/Dbg_Printf |"""
    w(name, "module_analysis.md", f"# 模块分析（导航）\n{modules}")

    w(name, "protocol_analysis.md", """# 协议分析（U13T，权威：读卡模组使用说明书）
- 帧格式: 7F(头) + 长度 + 地址 + 命令码 + 参数 + 校验(异或)；参数中 0x7F 双写转义
- 使用命令: 读卡号 0x10/响应 0x90；读块 0x11/响应 0x91；设波特率 0x2C/响应 0xAC
- 状态码: 0x00 正确/0xFF 无卡/0xFE 错误/0xFB 校验错误
- 波特率: 默认 9600，上电发 0x2C 切 115200（模块可能记忆，本机无条件跟随）
- 读块: 单块 16 字节；块 4 有数据播块 4，空则回退块 1；不做多块拼接
- TTS: USART2 9600，GBK 编码直接发送；指令 <S>3/<V>6/<I>0（TTS_SetupDefaults）
""")

    w(name, "data_flow.md", """# 数据流（导航）
## 读卡链路
USART1 RX 中断 → HAL_UART_RxCpltCallback（Card.c）→ 帧解析 UartReceiveCommand → card_res_flag
→ RFID 状态机（EXIST→WAIT→RESDATA→LEDLIGHT→NONE）→ chinese_data[16]
→ RfidLogic_Process（触发匹配/计数/去重）→ 事件 → TTS 播报（USART2）/ motor_trigger_flag
## 电机链路
电位器 ADC 采样 → MotorLogic_CalcStopTime → MotorLogic_Step（绝对计时状态机）→ Motor_Control → TIM2 PWM
## 触发停车链路
RfidLogic 返回 EV_TRIGGER_STOP → motor_trigger_flag=1 → MotorLogic_Step 消费 → STOPPING→WAIT→RAMPUP
""")

    w(name, "data_structures.md", f"""# 核心数据结构
- `trigger_rule_t`（config.h）: GBK 触发词规则（word/len/count_req/speak_en），表 TRIGGER_RULES，上限 TRIGGER_RULES_MAX=8
- `rfid_logic_t`（Task/rfid_logic.h）: 触发计数/去重状态（trig_count/trig_last_count_tick/trig_triggered/last_speak）
- `motor_logic_t`（Task/motor_logic.h）: 电机状态机状态（state/start_tick/state_tick/stop_time/speed/ramp_start/pending_trigger）
- `rfid_control_t`（Task/{"rfid_task.h" if is_rtos else "rfid_process.h"}）: 读卡状态机（chinese_data/block_num/wait_tick/led_tick/logic）
- `CMD`（hardware/rfid_card/Card.h）: 读卡帧缓冲（ReceiveBuffer[32]/block_data[16]）
- `card_res_flag`（CARD_FLAG_*）: 五态读卡标志（NONE/RESDATA/WAIT/EXIST/LEDLIGHT）
""")

    w(name, "hardcoded_params.md", f"""# 硬编码参数（导航，全部见 config.h，行号以 v0.5 为准）
| 参数 | 值 | 含义 |
|---|---|---|
| MOTOR_START_LATE_TIME_MS | 2000 | A 晚启动 |
| MOTOR_START_SLOW_TIME_MS | 4000 | B 缓启动 |
| MOTOR_SPEED_MAX | 999 | 速度上限 |
| MOTOR_MAX_RUN_TIME_MS | 1000000 | 1000s 上限 |
| MOTOR_TIME_START_S | 42 | E 降速起点 |
| MOTOR_SPEED_PERCENT | 50 | F 降速百分比 |
| MOTOR_TIME_DURATION_S | 5 | G 降速时长 |
| TRIGGER_RULES | 太阳/地球 GBK | 触发词表 |
| TRIGGER_COUNT_INTERVAL_MS | 10000 | 计数间隔 |
| TRIGGER_STOP_RAMP_TIME_S | 2 | H 减速时长 |
| TRIGGER_WAIT_TIME_S | 10 | I 静止等待 |
| LED_ON_TIME_S | 3 | C LED 熄灭延时 |
| SPEAK_DEDUP_TIME_S | 10 | D 去重窗口 |
| RFID_READ_DELAY_MS | 800 | 播报后延时 |
| RFID_LED_POLL_MS | 10 | LED 轮询 |
| RFID_READ_TIMEOUT_MS | 20 | 读块超时 |
| RES_MAX | 5000 | 电位器量程 |
| STOP_TIME_MIN_MS/MAX_MS | 10000/600000 | 停车时间范围 |
| DBG_ECHO_* | 1 | 调试输出开关 |
""")

    w(name, "issues_and_risks.md", f"""# 已知问题与风险（导航）
- 已修复（v0.3/v0.5）：开机 ORE 锁死（提前使能接收+ErrorCallback 自愈）；帧长>31 越界写；0xAC 误判读块；chinese_data 越界读（强制 0 结尾）；trig 数组 [4]→TRIGGER_RULES_MAX；100Hz 节拍 WAIT 超时漂移（改真实时间差）；NONE 态依赖自动上报失联（加 200ms 轮询）
- 遗留低危: RX 0x7F 反转义缺失（GBK 不含 0x7F）；speak_en=0 仅取消强制播报；IDLE/RAMPUP 期触发延迟到 RUN 消费；SLOW 窗口重启后可二次进入；绝对计时下停车重启后可能立即 STOP（设计既定）；UartSendCommand 转义依赖命令不含 0x7F
""")

def gen_esp32(name, is_rtos):
    c = proj_ctx(name)
    funcs = funcs_of(name)
    rt = "RTOS 版（双任务）" if is_rtos else "裸机版（单任务超级循环）"

    w(name, "project_overview.md", f"""# 项目概览（导航）
- 芯片: **ESP32-S3**（Xtensa LX7 双核，240MHz，512KB SRAM）
- 版本: {rt}；构建: ESP-IDF v6.0.2（idf.py，newlib，USB-Serial-JTAG console）
- 功能: 读卡(U13T) + TTS 播报 + 电机控制（多触发词停车/降速/电位器停止）+ LED + 调试输出
- 业务源文件: {len(c['business_sources'])} 个；头文件: {len(c['header_files'])} 个；业务函数: {len(funcs)}
- 组件: components/common 共享（config/pins/驱动/纯逻辑），main 仅应用差异文件
- 有效宏: {', '.join(c['active_macros'])}
""")

    w(name, "clock_tree.md", f"""# 时钟
- 芯片主频 240MHz（ESP32-S3 默认，IDF 自动配置）
- 电机 PWM: LEDC 20kHz 10-bit（LEDC_AUTO_CLK，sources/components/common/motor_drv.c）
- 时间基准: esp_timer_get_time()（64 位 us，应用层截断为 ms）
""")

    w(name, "gpio_and_pins.md", f"""# GPIO 与引脚（导航，统一在 components/common/pins.h）
| 引脚 | 功能 | 证据 |
|---|---|---|
| GPIO1 | 电位器 ADC1_CH0 | components/common/pins.h:ADC_CHANNEL |
| GPIO2 | LED1（低电平亮） | components/common/pins.h:PIN_LED1 |
| GPIO4 | 电机 LEDC CH0 | components/common/pins.h:PIN_MOTOR_PWM1 |
| GPIO5 | 电机 LEDC CH1 | components/common/pins.h:PIN_MOTOR_PWM2 |
| GPIO8 | LED2 | components/common/pins.h:PIN_LED2 |
| GPIO9 | LED3 | components/common/pins.h:PIN_LED3 |
| GPIO10 | UART1 TX 读卡 | components/common/pins.h:PIN_RFID_TX |
| GPIO11 | UART1 RX 读卡 | components/common/pins.h:PIN_RFID_RX |
| GPIO12 | UART2 TX TTS | components/common/pins.h:PIN_TTS_TX |
| GPIO13 | UART2 RX TTS | components/common/pins.h:PIN_TTS_RX |
| USB-DP/DM | USB-Serial-JTAG console | sdkconfig.defaults:CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG |
""")

    w(name, "peripheral_config.md", f"""# 外设配置（导航）
| 外设 | 配置 | 关键参数 | 证据 |
|---|---|---|---|
| UART1 | 读卡 9600→115200 | 环形缓冲 256B；init 后发 0x2C 切 115200 | components/common/card_uart.c |
| UART2 | TTS 9600 | 8N1；TTS_Send/TTS_SetupDefaults | components/common/tts.c |
| LEDC | 电机双通道互补 | 20kHz 10-bit；speed=0 两路 DUTY_MAX | components/common/motor_drv.c |
| ADC | 电位器 ADC1_CH0 | adc_oneshot，12-bit，衰减 12dB | components/common/adc.c |
| GPIO | LED×3 | 推挽+上拉 初始高 | components/common/led.c |
| USB-Serial-JTAG | console 调试 | printf 输出 | sdkconfig.defaults |
""")

    w(name, "nvic_priorities.md", f"""# 中断/调度（ESP-IDF FreeRTOS）
- USART 驱动: 中断内部处理（环形缓冲），应用层轮询消费，无显式优先级配置
- 任务优先级（RTOS 版）: RFID_Task=5，Motor_Task=1
- {'裸机版: app_main 单任务超级循环，vTaskDelay(1)=10ms（FreeRTOS 默认 100Hz）' if not is_rtos else 'FreeRTOS 节拍 100Hz，vTaskDelay(1)=10ms'}
""")

    w(name, "system_init_and_tasks.md", f"""# 启动顺序与任务
## 启动顺序（app_main）
LED_Init → Motor_Drv_Init → Motor_Control(0) → Dbg_Init → Card_Uart_Init（含波特率切换）→ TTS_Init → ADC_Init
→ {'xTaskCreate(RFID_Task pri=5, Motor_Task pri=1)' if is_rtos else 'Motor_Init + RFID_Init → while(1){RFID_Process; Motor_Process; vTaskDelay(1)}'}
## 任务（RTOS 版）
| 任务 | 优先级 | 栈 | 职责 |
|---|---|---|---|
| RFID_Task | 5 | 4096B | 读卡/播报/LED/触发（rfid_task.c） |
| Motor_Task | 1 | 2048B | 电机状态机（motor_task.c，喂 motor_logic） |
""")

    w(name, "memory_layout.md", f"""# 内存布局
- Flash: 2MB（分区 factory 1MB，固件约 237KB）
- SRAM: 512KB（RTOS 版 bss≈11.7KB 量级，充裕）
- 栈: RFID_Task 4096B / Motor_Task 2048B（RTOS 版）；main 任务默认 3584B（裸机版）
""")

    modules = f"""| 模块 | 文件 | 职责 | 核心函数 |
|---|---|---|---|
| 读卡驱动 | components/common/card_uart.c|h | UART1 驱动+命令组帧+波特率切换 | Card_Uart_Init/ReadCard/ReadBlock/Poll |
| 帧解析 | components/common/card_parse.c|h | U13T 帧解析纯逻辑 | UartReceiveCommand/Card_Parse_Feed |
| 触发/去重逻辑 | components/common/rfid_logic.c|h | 触发词匹配/计数/去重（四版共享） | RfidLogic_Process/TriggerMatch |
| 电机状态机 | components/common/motor_logic.c|h | 电机时序纯逻辑（四版共享） | MotorLogic_Step/CalcStopTime |
| 电机驱动 | components/common/motor_drv.c|h | LEDC 双通道互补 | Motor_Control/Motor_Drv_Init |
| TTS | components/common/tts.c|h | 发送+默认设置 | TTS_Send/SetupDefaults/Init |
| LED/ADC/DEBUG | components/common/led.c adc.c debug.c | 灯控/电位器/console 输出 | LED_Sta/Get_ADC_Value/Dbg_Printf |
| 读卡应用 | main/{'rfid_task.c' if is_rtos else 'rfid_process.c'}|h | 五态状态机+播报 | RFID_Task/RFID_Process |
| 电机应用 | main/{'motor_task.c' if is_rtos else 'motor_process.c'}|h | 采样+喂 motor_logic | Motor_Task/Motor_Process |
| 入口 | main/app_main.c | 初始化+调度 | app_main |"""
    w(name, "module_analysis.md", f"# 模块分析（导航）\n{modules}")

    w(name, "protocol_analysis.md", """# 协议分析（U13T，权威：读卡模组使用说明书）
- 帧格式: 7F(头) + 长度 + 地址 + 命令码 + 参数 + 校验(异或)；参数中 0x7F 双写转义
- 使用命令: 读卡号 0x10/响应 0x90；读块 0x11/响应 0x91；设波特率 0x2C/响应 0xAC
- 波特率: 9600→115200（card_uart.c 初始化切换）
- 读块: 单块 16 字节；块 4 → 回退块 1
- TTS: UART2 9600 GBK；指令 <S>3/<V>6/<I>0
""")

    w(name, "data_flow.md", """# 数据流（导航）
## 读卡链路
UART1 环形缓冲 → Card_Uart_Poll（每圈轮询）→ Card_Parse_Feed → card_res_flag
→ RFID 状态机 → chinese_data[16] → RfidLogic_Process → 事件 → TTS_Send / motor_trigger_flag
## 电机链路
ADC 采样 → MotorLogic_CalcStopTime → MotorLogic_Step → Motor_Control → LEDC PWM
## 触发停车链路
RfidLogic EV_TRIGGER_STOP → motor_trigger_flag → MotorLogic_Step 消费 → STOPPING→WAIT→RAMPUP
""")

    w(name, "data_structures.md", f"""# 核心数据结构
- `trigger_rule_t`（components/common/config.h）: 触发词规则，TRIGGER_RULES_MAX=8
- `rfid_logic_t` / `motor_logic_t`: 与 STM32 版逐字相同（四版共享）
- `rfid_control_t`（main/{"rfid_task.h" if is_rtos else "rfid_process.h"}）: wait_tick/wait_resend_times/led_tick/logic
- `CMD`（components/common/card_parse.h）: ReceiveBuffer[32]/block_data[16]
- 时间基准: esp_timer_get_time()/1000（32 位截断，差值比较回绕安全）
""")

    w(name, "hardcoded_params.md", f"""# 硬编码参数（导航，全部见 components/common/config.h + pins.h）
| 参数 | 值 | 含义 |
|---|---|---|
| MOTOR_START_LATE_TIME_MS | 2000 | A 晚启动 |
| MOTOR_START_SLOW_TIME_MS | 4000 | B 缓启动 |
| MOTOR_TIME_START_S | 42 | E 降速起点 |
| MOTOR_SPEED_PERCENT | 50 | F 降速百分比 |
| MOTOR_TIME_DURATION_S | 5 | G 降速时长 |
| TRIGGER_RULES | 太阳/地球 | 触发词表 |
| TRIGGER_STOP_RAMP_TIME_S | 2 | H 减速时长 |
| TRIGGER_WAIT_TIME_S | 10 | I 静止等待 |
| LED_ON_TIME_S | 3 | C |
| SPEAK_DEDUP_TIME_S | 10 | D |
| RFID_READ_TIMEOUT_MS | 20 | 读块超时 |
| MOTOR_PWM_FREQ_HZ | 20000 | 电机 PWM 频率 |
| MOTOR_PWM_RES_BITS | 10 | LEDC 分辨率 |
| RES_MAX/STOP_TIME_MIN/MAX | 5000/10s/600s | 电位器 |
| 引脚 | GPIO1/2/4/5/8/9/10/11/12/13 | pins.h |
""")

    w(name, "issues_and_risks.md", f"""# 已知问题与风险（导航）
- 已修复（v0.5）：100Hz 节拍 WAIT 超时漂移 10 倍（改真实时间差）；NONE 态依赖模块自动上报失联（加 200ms 轮询）；发送 0x7F 转义分支重写；初始化 API 返回值检查
- 遗留低危: RX 0x7F 反转义缺失（GBK 不含 0x7F）；speak_en=0 语义；IDLE/RAMPUP 期触发延迟；SLOW 窗口重启后二次进入；绝对计时停车重启后立即 STOP（设计既定）
- 中文路径陷阱: picolibc specs 不支持中文路径 → sdkconfig.defaults CONFIG_LIBC_NEWLIB=y（勿删）
""")

def main():
    gen_stm32("stm32_rfid_tts_rtos", True)
    gen_stm32("stm32_rfid_tts_baremetal", False)
    gen_esp32("esp32_tts_rtos", True)
    gen_esp32("esp32_tts_baremetal", False)
    print("_analysis 生成完成（4 项目 × 13 文件）")

if __name__ == "__main__":
    main()
