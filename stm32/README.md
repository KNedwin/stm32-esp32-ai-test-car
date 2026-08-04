# 新能源小车 - STM32 读卡语音播报控制系统

本项目是"读卡语音播报程序源码4.0-案例"的复刻工程，包含 **两个并列的独立项目**：

| 目录 | 版本 | 说明 |
|---|---|---|
| `rfid_tts_rtos/` | RTOS 版 | 使用 FreeRTOS（CMSIS V1 接口），与案例源码结构一致（另新增 USART3 调试输出口） |
| `rfid_tts_baremetal/` | 裸机版 | 不使用操作系统，主循环 + 状态机实现（另新增 USART3 调试输出口） |

两项目功能完全相同、硬件接线完全相同，仅内部实现方式不同。

---

## 1. 功能清单

| # | 功能 | 说明 |
|---|---|---|
| 1 | 晚启动 + 缓启动 | 通电延时 A 秒后，电机用 B 秒线性缓启动至目标速度 |
| 2 | 实时读卡 + LED | 任意时刻轮询读卡；读到卡 LED 亮；卡在线圈上持续亮，卡脱离 C 秒后熄灭 |
| 3 | TTS 语音播报 | 卡内 GBK 数据送语音模块播报；D 秒内相同内容不重复播报 |
| 4 | 定时降速 | 电机运行到第 E 秒时速度降至 F%，持续 G 秒后恢复目标速度 |
| 5 | 触发停车 | **多触发词规则表**（config.h 可配）：如"太阳"第 1 次读到即播报+停车；"地球"第 2 次读到（两次间隔≥10s）才播报+停车；停车序列 = 2 秒减速 + 10 秒静止后重新缓启动；每个词整个上电周期只触发一次，之后只播报亮灯 |
| 6 | 电位器自动停止 | 根据电位器阻值计算停车时间（10s ~ 600s），到时自动停车 |
| 7 | 数据输出串口 | USART3 实时输出读卡数据、电机状态、LED 状态 |
| 8 | 系统保护 | 程序卡死（HardFault）自动重启 |

所有可调参数集中在一个文件 `config.h` 中。

---

## 2. 硬件接线总表（两版相同）

主控板：STM32F103C8T6（LQFP48 封装，64KB Flash，20KB RAM，8MHz 晶振）

> ⚠️ 芯片前提说明：**案例源码工程**按 STM32F103C6T6A（32KB Flash / 10KB RAM）生成；**本项目复刻时按 C8T6（64KB / 20KB）设计**。若你手头实际芯片是 C6（10KB RAM），RTOS 版的 FreeRTOS 堆必须改回 ≤6368，否则 RAM 超限。

| 功能 | 引脚 | 外设 | 参数 |
|---|---|---|---|
| 读卡模块 U13T（RFID） | PA9 = TX，PA10 = RX | USART1 | 9600，初始化后切 115200 |
| 语音模块 CN-TTS | PA2 = TX，PA3 = RX | USART2 | 9600，8N1 |
| 数据输出串口（调试） | PB10 = TX，PB11 = RX | USART3 | 115200，8N1，只用 TX |
| 电机 PWM 输出 1 | PA0 | TIM2_CH1 | PWM Mode 2，1kHz |
| 电机 PWM 输出 2 | PA1 | TIM2_CH2 | PWM Mode 2，1kHz |
| 电位器（停车时间调节） | PB1 | ADC1_IN9 | 28.5 周期采样，连续转换 |
| LED 指示灯（三路并联兼容） | PC13、PB12、PA8 | GPIO 推挽输出 | 低电平点亮，初始高电平 |
| 调试下载口（SWD） | PA13 = SWDIO，PA14 = SWCLK | — | — |
| 外部晶振 | PD0 = OSC_IN，PD1 = OSC_OUT | HSE | 8MHz |

接线注意：
- 串口均为 **TX 接对方 RX、RX 接对方 TX**（交叉连接）
- 三个 LED 引脚兼容（接任意一个或三个都接均可，程序同时驱动）
- **TTS 模块必须 5V 供电（4.5~5.5V，播报时峰值 320mA，需独立供电或稳压）**；**U13T 读卡模块供电范围 3.0~5.5V**（3.3V 即可工作）
- 若模块以 5V 供电，其 TX 高电平接近 5V：PA10/PA3/PB11 均为 5V 容忍（FT）引脚，可直接连接，无需电平转换
- 所有模块 GND 与主控共地

---

## 3. 两版实现对比

| 对比项 | RTOS 版（rfid_tts_rtos） | 裸机版（rfid_tts_baremetal） |
|---|---|---|
| 调度方式 | FreeRTOS 内核，两个任务并行调度 | 主循环 `while(1)` 顺序执行两个状态机 |
| CubeMX 时基 | TIM1（SysTick 交给 FreeRTOS） | SysTick |
| Middleware | FreeRTOS（CMSIS_V1） | 无 |
| 延时写法 | `vTaskDelay()` 可随意阻塞 | `HAL_GetTick()` 时间差非阻塞 |
| 任务/状态机 | RFID_Task（高优先级 1024 字栈）、Motor_Control_Task（空闲优先级 512 字栈） | `RFID_Process()`、`Motor_Process()` 两个函数 |
| 实时性 | TTS 发送阻塞只卡自己任务，电机任务不受影响 | 需把延时全部改非阻塞；TTS 发送期间电机 PWM 有短暂停顿（毫秒级，可感知轻微顿挫但不影响功能） |
| 内存占用 | 额外约 8~9KB（FreeRTOS 堆 8192B + 内核/空闲任务开销） | 无 |
| 适用场景 | 逻辑复杂、任务多、需要并行阻塞 | 逻辑简单、资源紧张、便于阅读 |

**选择建议**：学习案例原结构、后续可能加复杂逻辑 → RTOS 版；追求简单可控、代码量小 → 裸机版。

---

## 4. 阅读指引（重要）

### 完全没接触过 STM32CubeMX 的读者，请按顺序阅读：

1. 先读你所用版本的 `docs/01-CubeMX从零教程.md` —— 手把手教你用 STM32CubeMX 生成工程
2. 再读 `docs/02-项目说明.md` —— 理解功能规格、参数含义、代码架构

### 已有 CubeMX 基础、只想快速上手的读者：

1. 直接看 `docs/01-CubeMX从零教程.md` 附录的"配置速查表"
2. 再读 `docs/02-项目说明.md` 的 config.h 参数部分

---

## 5. 编译与烧录概述（详细步骤见各版教程）

| 场景 | 命令/操作 |
|---|---|
| Windows + Keil MDK | 用 CubeMX 生成 MDK-ARM 工程后双击 `.uvprojx` 打开编译下载 |
| Windows + STM32CubeIDE | CubeMX 生成 STM32CubeIDE 工程，IDE 内编译运行 |
| Linux 本机（推荐验证用） | 用 CubeMX 生成 **CMake** 工具链工程，拷贝到 Linux 后按 01 教程 §16.3 调整工具链路径，然后 `cmake -B build && cmake --build build`，`st-flash write build/*.bin 0x08000000` |

依赖工具：arm-none-eabi-gcc（已安装 14.2.1）、CMake（已安装 4.2.3）、st-flash（已安装 1.8.0）。

> ⚠️ 注意：CubeMX 在 Windows 上生成的 CMake/Makefile 工程会硬编码 Windows 工具链绝对路径，拷贝到 Linux 后需按 01 教程 §16.3 修改（详见各版教程）。

---

## 6. 目录结构

```
stm32/
├── README.md                          # 本文件
├── rfid_tts_rtos/                     # RTOS 版
│   ├── docs/
│   │   ├── 01-CubeMX从零教程.md
│   │   └── 02-项目说明.md
│   ├── config.h                       # 全部可调参数
│   ├── Core/                          # CubeMX 生成的外设代码
│   ├── Drivers/                       # HAL 库 + CMSIS（以 CubeMX 生成/随 CubeMX 工具链为准，勿用案例的 C6 版本覆盖）
│   ├── Middlewares/                   # FreeRTOS 源码（RTOS 版独有）
│   ├── hardware/                      # 自写驱动：USART/RFID/PWM/LED/ADC/DEBUG
│   ├── Task/                          # 任务代码（RTOS 版）
│   ├── CMakeLists.txt / *.ld / startup*.s
└── rfid_tts_baremetal/                # 裸机版
    ├── docs/
    │   ├── 01-CubeMX从零教程.md
    │   └── 02-项目说明.md
    ├── config.h
    ├── Core/  Drivers/  hardware/  Task/
    └── CMakeLists.txt / *.ld / startup*.s
```

> 📌 当前状态：本项目处于**文档先行**阶段，`config.h`、`Core/`、`Drivers/`、`hardware/`、`Task/` 等代码目录由后续步骤生成——你在 Windows 上用 CubeMX 生成工程骨架后，代码文件将填充进对应目录（详见各版 01 教程 §15）。

---

## 7. 硬件设备说明

### U13T 读卡模组（读卡模块）
- 13.56MHz 非接触读卡，ISO14443A/B 协议，支持 S50 等 Mifare 卡
- 供电范围 3.0~5.5V
- 串口默认 9600 波特率；本程序上电后发送命令将其切换为 115200（模块可能记忆 115200，切换流程幂等，无需人工干预）
- 命令帧格式：`0x7F(帧头) + 长度 + 地址 + 命令码 + 参数 + 校验(异或)`
- 详细命令见《读卡模组使用说明书》

### CN-TTS 语音合成模块（TTS 模块）
- 串口 9600 波特率，直接发送 GBK 编码中文即自动播报
- 控制指令：`<S>3` 语速、`<V>6` 音量、`<I>0` 上电提示（`<I>` 指令同时开启模块设置断电保存——注意：这是 TTS 模块自身指令的官方描述，与本项目读卡模块波特率切换无关）
- 5V 供电（4.5~5.5V），播报时电流最大 320mA，需独立供电或稳压

---

## 8. 常见问题速查

| 问题 | 排查方向 |
|---|---|
| 编译报错找不到 `stm32f1xx_hal.h` | 检查头文件搜索路径是否包含 Drivers 各目录 |
| 烧录失败 | 检查 ST-Link 接线（SWDIO/SWCLK/GND/3.3V）、板子供电 |
| 串口无输出 | 检查波特率（USART3=115200）、TX/RX 是否交叉、共地 |
| 电机不转 | 检查 TIM2 是否 PWM Mode 2、PA0/PA1 接线、`MOTOR_TARGET_SPEED` 是否为 0 |
| 读卡没反应 | 确认波特率切换代码（教程 §15.2）已加入；检查读卡模块供电（3.0~5.5V）与共地；卡片是否在感应区 |
