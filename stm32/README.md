# 新能源小车 - STM32 读卡语音播报控制系统

本项目是「读卡语音播报程序源码4.0-案例」的复刻工程，包含 **两个并列的独立项目**：

| 目录 | 版本 | 说明 |
|---|---|---|
| rfid_tts_rtos/ | RTOS 版 | FreeRTOS（CMSIS V1，HAL 时基 TIM1），与案例源码结构一致 |
| rfid_tts_baremetal/ | 裸机版 | 主循环 + 双状态机（SysTick 时基），无操作系统 |

两项目功能完全相同、硬件接线完全相同，仅内部实现方式不同。在案例基础上新增：**运行时参数层 + USART3 参数 CLI + PC 上位机 + 电机转向可配 + ISP 免拆烧录**。

---

## 1. 功能清单

| # | 功能 | 说明 |
|---|---|---|
| 1 | 晚启动 + 缓启动 | 通电延时后电机线性缓启动至目标速度 |
| 2 | 实时读卡 + LED | 任意时刻轮询读卡；卡在线圈上 LED 持续亮，脱离数秒后熄灭 |
| 3 | TTS 语音播报 | 卡内 GBK 数据送 CN-TTS 播报，窗口期内相同内容不重复；上电提示音 7 号（<I>7） |
| 4 | 定时降速（多段） | 支持最多 8 个减速窗口 [第几秒/持续秒/百分比]，不重叠、按时间升序生效 |
| 5 | 多触发词停车 | 规则表可配：一次性词与计数型词；触发即播报 + 停车序列（2s 减速 → 10s 静止 → 缓启动）；每词整个上电周期只触发一次 |
| 6 | 参数化自动停车 | 自动停车总时长可配（默认 300s，范围 10~1000s），掉电保存（原电位器方案已退役） |
| 7 | 电机转向可配 | motor_dir 正转/反转（PWM 双路角色交换） |
| 8 | USART3 数据输出 + 参数 CLI | 实时输出读卡/电机/LED 状态；同一串口接收行协议命令修改全部参数并存内部 Flash 末页 |
| 9 | 系统保护 | HardFault 自动重启；看门狗 |

---

## 2. 硬件接线总表（两版相同）

主控板：STM32F103C8T6（LQFP48，64KB Flash，20KB RAM，8MHz 晶振）

> ⚠️ 案例工程按 C6（32KB/10KB）生成，本项目按 C8T6 设计。若用 C6 芯片，RTOS 版 FreeRTOS 堆须改回 ≤6368，否则 RAM 超限。

| 功能 | 引脚 | 外设 | 参数 |
|---|---|---|---|
| 读卡模块 U13T（RFID） | PA9 = TX，PA10 = RX | USART1 | 9600，初始化后切 115200 |
| 语音模块 CN-TTS | PA2 = TX，PA3 = RX | USART2 | 9600，8N1 |
| 调试/参数口 | PB10 = TX，PB11 = RX | USART3 | 115200，8N1（TX 输出状态，RX 收 CLI 命令） |
| 电机 PWM 输出 1 / 2 | PA0 / PA1 | TIM2_CH1 / CH2 | PWM Mode 2，1kHz（转向=双路角色交换） |
| LED 指示灯（三路并联兼容） | PC13、PB12、PA8 | GPIO 推挽 | 低电平点亮 |
| 调试下载口（SWD） | PA13 / PA14 | — | — |
| 外部晶振 | PD0 / PD1 | HSE | 8MHz |

> 电位器（PB1/ADC1_IN9）已退役：停车时长改为参数配置，PB1 悬空即可；ADC 外设代码保留但无调用方。

接线注意：串口均 TX↔RX 交叉连接；三 LED 引脚兼容可任接；**CN-TTS 必须 5V 独立供电**（峰值 320mA）；U13T 供电 3.0~5.5V；模块 5V 供电时 TX 接 PA10/PA3/PB11（均 FT 引脚）可直连；全部共地。

---

## 3. 两版实现对比

| 对比项 | RTOS 版 | 裸机版 |
|---|---|---|
| 调度 | FreeRTOS 两任务并行 | while(1) 顺序执行两个状态机 |
| CubeMX 时基 | TIM1 | SysTick |
| 任务/状态机 | RFID_Task（1024 字栈）、Motor_Control_Task（512 字栈） | RFID_Process() / Motor_Process() |
| 实时性 | TTS 阻塞只卡自己任务 | TTS 发送期间毫秒级停顿（可接受） |
| 内存 | 额外约 8~9KB（FreeRTOS 堆 8192B） | 无 |

---

## 4. 参数配置系统（阶段一~四成果）

### 4.1 参数层（nvs_params）

- params_t 结构与 ESP32 版**完全同构**：电机时序、转向、减速窗口×8、触发词规则×8、LED/去重/轮询、自动停车时长等
- 存储：内部 Flash 末页 0x0800FC00（1KB），magic 0xA55A + CRC16 校验，失败自动回落 config.h 默认宏
- 上电 params_init() → params_apply() 注入逻辑层；每条 CLI 修改命令即时重新注入

### 4.2 USART3 行协议 CLI（115200，CRLF 结尾）

```text
GET all                 回显全部参数（>W/>R 多行 + END）
SET late_s 2            标量赋值（秒单位，固件×1000）
SET dir 1               电机转向 0=正转 1=反转
SET win_add 42 5 50     追加减速窗口（第几秒/持续秒/百分比）
SET win_del <idx>       删除窗口
SET rule_add <gbk_hex> <count> <speak>   触发词为 GBK hex
SET rule_del <idx>      删除触发词
SAVE                    sanitize 后写 Flash 末页
DUMP                    十六进制 dump 存储页（诊断）
ISP                     软跳系统 Bootloader（见 §5）
REBOOT                  软复位
应答：OK:<value> / ERR:unknown / ERR:range（以 > 前缀区分于状态输出）
```

### 4.3 PC 上位机（tools/stm32_host）

```bash
cd tools/stm32_host && python host.py   # http://127.0.0.1:8321，自动开浏览器
```
- COM 口自动枚举（pyserial），网页表单与 ESP32 版同构；触发词在 PC 侧 UTF-8→GBK hex 再下发
- 工具页签：[编译] 经 wsl.exe 调 CMake 构建；[烧录] 支持 ST-Link 或 串口 ISP（先发 ISP 命令再 stm32flash）

---

## 5. ISP 免拆烧录（备选能力）

CLI 发送 ISP → 关中断跳转出厂系统 Bootloader（0x1FFFF000）→ PC 端 stm32flash 串口刷固件，无需 ST-Link/BOOT0 跳线。⚠️ 跳转前需拔掉 U13T 读卡模块（其 RX 占用 USART1 干扰应答）。日常开发仍推荐 ST-Link。

---

## 6. 编译与烧录

```bash
cd stm32/rfid_tts_rtos            # 或 rfid_tts_baremetal
cmake --preset Debug && cmake --build build/Debug
st-flash write build/Debug/<工程名>.bin 0x08000000   # C8T6 Flash 基址
screen /dev/ttyUSB0 115200                           # USART3 调试/CLI 口
```

工具链 arm-none-eabi-gcc 14.2.1 / CMake / st-flash；工具链文件 cmake/gcc-arm-none-eabi.cmake 无路径硬编码，Linux 直接可编译。CubeMX 重新生成工程后需重做 AGENTS.md 所列 4 处修改（业务代码在独立目录与 USER CODE 区，不受影响）。

---

## 7. 主机单元测试（改逻辑后必跑）

```bash
cd stm32/tests && ./run_tests.sh
```

当前 **109 项全绿**：Card.c 读卡 25 + rfid_logic 32 + motor_logic 30 + 电机仿真 22。

---

## 8. 目录结构

```text
stm32/
├── README.md                        # 本文件
├── docs/串口配置模式实施计划.md      # 参数化/CLI/上位机 设计与验收清单
├── tests/                           # 主机单元测试（run_tests.sh，109 项）
├── rfid_tts_rtos/
│   ├── docs/01-CubeMX从零教程.md、02-项目说明.md
│   ├── config.h                     # 全部默认参数宏（回落值来源）
│   ├── config/                      # nvs_params / param_cli / isp_jump
│   ├── Core/ Drivers/ Middlewares/  # CubeMX 生成物
│   ├── hardware/                    # 自写驱动：USART/RFID/PWM/LED/DEBUG（ADC 为遗留）
│   └── Task/                        # 任务 + 共享逻辑层（rfid_logic/motor_logic）
└── rfid_tts_baremetal/              # 同构（无 Middlewares）
```

---

## 9. 当前状态

- ✅ 阶段一~四已完成并合入：参数层 + Flash 末页存储、USART3 CLI、ISP 软跳转、PC 上位机；双版交叉编译绿 + 单测 109 项全绿
- ⏳ 阶段五上板联调待板卡到手，验收清单见 docs/串口配置模式实施计划.md

---

## 10. 硬件设备说明与常见问题

- **U13T 读卡模组**：13.56MHz ISO14443A/B；串口 9600 上电切 115200（幂等）；帧格式 0x7F + 长度 + 地址 + 命令 + 参数 + 异或校验
- **CN-TTS 模块**：串口 9600 直发 GBK 即播报；<S>3 语速、<V>6 音量、<I>7 上电提示音 7 号（<I> 指令同时启用断电保存）
- FAQ：找不到 HAL 头文件→查 include 路径；烧录失败→查 SWD 接线；串口无输出→查 115200/交叉/共地；电机不转→查 PWM Mode 2 与 MOTOR 目标速度；读卡没反应→查波特率切换代码与供电
