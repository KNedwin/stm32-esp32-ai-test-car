# 新能源小车语音播报系统（RFID 读卡 + TTS 播报 + 电机控制)

以「读卡语音播报程序源码4.0」案例工程为原型，在 **STM32F103C8T6** 与 **ESP32-S3** 两套平台上完成的复刻与增强工程。每个平台提供 **RTOS 版**与**裸机版**两个并列实现，**四版功能规格完全一致**，共享同一份纯业务逻辑层（rfid_logic.c 触发词/去重/计数，motor_logic.c 电机状态机）。

## 功能特性

| # | 功能 | 说明 |
|---|---|---|
| 1 | 晚启动 + 缓启动 | 通电延时后电机线性缓启动至目标速度（时序参数集中在 config.h） |
| 2 | 实时读卡 + LED | 任意时刻轮询读卡；卡在线圈上 LED 持续亮，脱离数秒后熄灭 |
| 3 | TTS 语音播报 | 卡内 GBK 数据送 CN-TTS 模块播报，窗口期内相同内容不重复 |
| 4 | 定时降速 | 运行到指定秒数降至百分比速度，持续数秒后恢复 |
| 5 | 多触发词停车 | 触发词规则表可配：一次性词与计数型词（两次有效计数有最小间隔）；触发即播报 + 停车序列（减速 → 静止 → 重新缓启动），每个词整个上电周期只触发一次 |
| 6 | 参数化自动停车 | 自动停车时长可配（默认 5 分钟）：ESP32 经配网网页设置，STM32 经 USART3 串口 CLI 设置，掉电保存 |
| 7 | 数据输出口 | STM32 = USART3 @115200；ESP32 = USB-Serial-JTAG（免 USB 转 TTL） |
| 8 | 系统保护 | HardFault 自动重启 / 看门狗 |

## 四版工程一览

| 目录 | 平台 | 调度 | 说明 |
|---|---|---|---|
| stm32/rfid_tts_rtos/ | STM32F103C8T6 | FreeRTOS（CMSIS_V1，HAL 时基 TIM1） | 与案例结构一致的复刻 + 增强 |
| stm32/rfid_tts_baremetal/ | STM32F103C8T6 | 主循环 + 状态机（SysTick 时基） | 无操作系统实现 |
| esp32/esp32_tts_rtos/ | ESP32-S3 | 双 FreeRTOS 任务 | STM32 RTOS 版的移植 |
| esp32/esp32_tts_baremetal/ | ESP32-S3 | 单任务超级循环 | STM32 裸机版的移植 |

> 共享逻辑：ESP32 两版共用 esp32/components/common/（config/pins + 驱动 + 纯逻辑层）；rfid_logic.c 与 motor_logic.c 在四个项目中保持逐字同步，漂移由主机单测脚本检查。

## 目录结构

```text
├── stm32/
│   ├── rfid_tts_rtos/         # STM32 RTOS 版（CubeMX 6.17 + CMake/GCC）
│   ├── rfid_tts_baremetal/    # STM32 裸机版
│   ├── tests/                 # 主机单元测试（109 项，无硬件依赖）
│   └── docs/                  # CubeMX 从零教程 / 项目说明 / 串口配置模式实施计划
├── esp32/
│   ├── esp32_tts_rtos/        # ESP32-S3 RTOS 版（ESP-IDF v6.0.2）
│   ├── esp32_tts_baremetal/   # ESP32-S3 裸机版
│   ├── components/common/     # 两版唯一维护点：config/pins + 驱动 + 纯逻辑层
│   ├── tests/                 # 主机单元测试（76 项，含共享层漂移检查）
│   └── docs/
├── tools/stm32_host/          # PC 上位机（Python + 网页，经 USART3 配置 STM32 参数）
└── docs/知识库/               # 四项目知识库（00_阅读指南 ~ 11_常见问题 + SUMMARY + 验收包）
```

## 硬件要点

- 读卡模块 U13T（RFID）、语音模块 CN-TTS、电机 PWM 双路、LED 三路并联（低电平亮）
- **CN-TTS 必须独立 5V 供电**（播报峰值约 320mA）；U13T 支持 3.0~5.5V
- 所有串口 **TX↔RX 交叉连接**；模块 5V 供电时其 TX 接主控 FT（5V 容忍）引脚可直连；全部共地
- 引脚分配详见 [stm32/README.md](stm32/README.md) 与 [esp32/README.md](esp32/README.md)

## 快速开始

### STM32（CMake + arm-none-eabi-gcc，Linux 直接可编译）

```bash
cd stm32/rfid_tts_rtos            # 或 rfid_tts_baremetal
cmake --preset Debug && cmake --build build/Debug
st-flash write build/Debug/<工程名>.bin 0x08000000   # C8T6 Flash 基址
screen /dev/ttyUSB0 115200                           # USART3 调试口
```

### ESP32-S3（ESP-IDF v6.0.2）

```bash
source ~/.espressif/tools/activate_idf_v6.0.2.sh     # 每次编译前必须激活（路径按本机安装调整）
cd esp32/esp32_tts_rtos           # 或 esp32_tts_baremetal
idf.py set-target esp32s3         # 首次执行一次
idf.py build
idf.py -p /dev/ttyACM0 flash monitor                 # USB-Serial-JTAG 口
```

> 注意：两版 sdkconfig.defaults 已配置 CONFIG_LIBC_NEWLIB=y 以支持中文路径，请勿删除。

### 主机单元测试（改逻辑后必跑，无硬件依赖）

```bash
cd stm32/tests && ./run_tests.sh   # Card.c 25 + rfid_logic 32 + motor_logic 30 + 电机仿真 22
cd esp32/tests && ./run_tests.sh   # rfid_logic 32 + card_parse 14 + motor_logic 30（含共享漂移检查）
```

## 参数配置

- **ESP32**：上电进 AP 配网网页，可设 WiFi、触发词开关、自动停车总时长等，存 NVS
- **STM32**：USART3 行协议 CLI（GET / SET / HELP），修改即时生效并存内部 Flash 末页
- **PC 上位机**：tools/stm32_host/，python host.py 启动（http.server :8321），网页界面经串口透传读写 STM32 参数

## 文档导航

| 文档 | 内容 |
|---|---|
| stm32/docs/01-CubeMX从零教程.md | CubeMX 生成流程 + 生成后补码步骤 |
| stm32/docs/02-项目说明.md、esp32/docs/02-项目说明.md |02-项目说明.md | 功能规格、config.h 全参数、状态机架构、验收清单 |
| stm32/docs/串口配置模式实施计划.md | 参数层/CLI/上位机设计与验收清单 |
| docs/知识库/<项目名>/ | 四套知识库（阅读指南 ~ 常见问题 + SUMMARY + 验收包） |

## 本地资料说明（不入库）

以下参考资料仅保存在本地、不随本仓库分发：

- 读卡模组使用说明书/ 、TTS语音播报合成模块使用说明书/ — 硬件协议权威资料
- 读卡语音播报程序源码4.0-案例/ — 原始参考案例工程，版权归原作者所有
- ESP32-S3 资料/ — 芯片手册、原理图与厂商工具
- AGENTS.md、.gitignore — 本地开发辅助文件
