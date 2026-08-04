# ESP32-S3 读卡语音播报控制系统（STM32 项目复刻）

本项目是 `stm32/` 下两个 STM32 工程（读卡 + TTS 播报 + 电机控制）的 **ESP32-S3 复刻版**，包含两个并列项目：

| 目录 | 版本 | 对应 STM32 工程 | 说明 |
|---|---|---|---|
| `esp32_tts_rtos/` | RTOS 版 | `stm32/rfid_tts_rtos` | 两个 FreeRTOS 任务（读卡任务 + 电机任务） |
| `esp32_tts_baremetal/` | 裸机版 | `stm32/rfid_tts_baremetal` | 单任务超级循环（顺序执行两个状态机） |

功能规格与 STM32 版**完全一致**（晚启动/缓启动、实时读卡 + LED 保持、TTS 播报去重、定时降速、多触发词停车、电位器自动停止、数据输出口、HardFault 等效保护）。

---

## 1. 开发环境（已就绪）

| 项 | 值 |
|---|---|
| ESP-IDF | **v6.0.2**（`IDF_PATH=<HOME>/.espressif/v6.0.2/esp-idf`） |
| 激活脚本 | `<HOME>/.espressif/tools/activate_idf_v6.0.2.sh` |
| 目标芯片 | **ESP32-S3**（Xtensa，512KB SRAM，双核） |
| 编译工具 | `idf.py`（ESP-IDF 自带） |

**每次编译前必须激活环境**：
```bash
source <HOME>/.espressif/tools/activate_idf_v6.0.2.sh
```

> ⚠️ **中文路径说明**：ESP-IDF 默认使用 picolibc，其 gcc specs 机制**不支持中文路径**（本项目位于含中文的目录，直接编译会报 `cannot read spec file`）。两版项目的 `sdkconfig.defaults` 已配置 `CONFIG_LIBC_NEWLIB=y`（关闭 picolibc）绕开该问题，**请勿删除该配置**。

**编译/烧录/监视**：
```bash
cd esp32/esp32_tts_rtos          # 或 esp32_tts_baremetal
idf.py set-target esp32s3        # 首次生成工程时执行一次
idf.py build                     # 编译
idf.py -p /dev/ttyACM0 flash     # 烧录（USB-Serial-JTAG 口）
idf.py -p /dev/ttyACM0 monitor   # 串口监视（USB-Serial-JTAG console）
```

---

## 2. 硬件接线表（ESP32-S3，引脚可在各项目 config.h 修改）

| 功能 | 引脚 | 外设 | 参数 |
|---|---|---|---|
| 读卡模块 U13T | GPIO10 = TX，GPIO11 = RX | UART1 | 9600，初始化后切 115200 |
| 语音模块 CN-TTS | GPIO12 = TX，GPIO13 = RX | UART2 | 9600，8N1 |
| 调试输出（console） | 原生 USB 口 | **USB-Serial-JTAG** | 免 USB 转 TTL，`idf.py monitor` 直接查看 |
| 电机 PWM 输出 1 | GPIO4 | LEDC 通道 0 | 20kHz，10-bit，互补双路 |
| 电机 PWM 输出 2 | GPIO5 | LEDC 通道 1 | 同上 |
| 电位器（停车时间调节） | GPIO1 | SARADC1 通道 0 | 12-bit |
| LED 指示灯（三路并联兼容） | GPIO2、GPIO8、GPIO9 | GPIO 推挽 | 低电平点亮，初始高电平 |
| USB-Serial-JTAG | 原生 USB 口 | console 调试输出 | `idf.py monitor` 查看 |

接线注意（与 STM32 版相同）：
- 串口均 **TX 接对方 RX、RX 接对方 TX**（交叉连接）
- **TTS 模块必须 5V 供电**（4.5~5.5V，播报峰值 320mA，需独立供电/稳压）；U13T 供电 3.0~5.5V
- 模块 5V 供电时其 TX 高电平接近 5V：ESP32-S3 大部分 GPIO **5V 容忍（FT）**，GPIO10/11/12/13 均为 FT，可直接连接
- 所有模块 GND 与 ESP32 共地

---

## 3. 两版实现对比

| 对比项 | esp32_tts_rtos（RTOS 版） | esp32_tts_baremetal（裸机版） |
|---|---|---|
| 调度 | 两个 FreeRTOS 任务并行（`xTaskCreate`） | `app_main` 单任务 `while(1)` 顺序执行 |
| 读卡流程 | `RFID_Task`（栈 4096B，优先级 5） | `RFID_Process()`（主循环每圈调用，非阻塞） |
| 电机流程 | `Motor_Task`（栈 2048B，优先级 1） | `Motor_Process()`（同上） |
| 延时 | `vTaskDelay` 可阻塞 | `esp_timer` 时间差非阻塞 |
| 实时性 | TTS 发送只卡自己任务 | TTS 发送期间主循环停顿（毫秒级，可接受） |
| 内存 | S3 512KB SRAM，无压力 | 更省 |

> 注：ESP-IDF 内建 FreeRTOS，即使"裸机版"也运行在调度器上（app_main 本身就是任务）；两版差异体现在**代码组织方式**（多任务 vs 超级循环），与 STM32 两版的对位关系一致。

---

## 4. 与 STM32 版的代码复用关系

| 模块 | 复用方式 |
|---|---|
| `config.h` | 逐字移植（触发词规则表、A~I 时序参数，全部一致） |
| `rfid_logic.c` | **逐字相同**（纯逻辑、无硬件依赖）→ 两 ESP32 项目共用，主机单元测试直接复用 |
| 触发词/去重/计数决策 | 已封装在 rfid_logic，零改动 |
| 电机状态机逻辑 | 移植（绝对计时基准从 `HAL_GetTick` 换成 `esp_timer_get_time()/1000`） |
| U13T 帧解析状态机 | 移植（含长度上限防护、"仅 0x91 产生事件"等修复），UART 底层换 ESP-IDF 驱动 |
| TTS 发送 | STM32 版用 printf→USART2；**ESP32 版改用 `uart_write_bytes(UART2)` 直接发送**，printf 归还给 console 调试输出（角色更清晰） |
| 数据输出协议 | `[SYS]/[RFID]/[LED]/[MOTOR]` 格式完全一致 |
| 主机测试 | `stm32/tests/` 的 rfid_logic 测试扩展到 esp32 版路径；新增 esp32/tests/ 帧解析测试 |

---

## 5. 目录结构（规划）

```
esp32/
├── README.md                          # 本文件
├── esp32_tts_rtos/                    # RTOS 版
│   ├── docs/01-项目说明.md
│   ├── CMakeLists.txt                 # 顶层（project + 引入 main）
│   ├── sdkconfig                      # 首次 set-target 生成
│   └── main/
│       ├── CMakeLists.txt
│       ├── config.h                   # 全部可调参数（移植）
│       ├── app_main.c                 # 入口：初始化 + 创建两任务
│       ├── rfid_task.c|h              # 读卡任务（状态机移植）
│       ├── motor_task.c|h             # 电机任务（绝对计时状态机移植）
│       ├── rfid_logic.c|h             # 纯逻辑层（与 STM32 版逐字相同）
│       ├── card_uart.c|h              # U13T 协议 + ESP-IDF UART 驱动
│       ├── tts.c|h                    # TTS 发送（UART2）
│       ├── motor_drv.c|h              # LEDC 电机驱动（Motor_Control 接口）
│       ├── led.c|h                    # LED
│       ├── adc.c|h                    # 电位器采样
│       └── debug.c|h                  # Dbg_Printf → console
├── esp32_tts_baremetal/               # 裸机版（结构同上，rfid_process.c / motor_process.c）
│   └── docs/01-项目说明.md
└── tests/                             # 主机单元测试（rfid_logic 复用 + 帧解析）
    ├── run_tests.sh
    ├── test_logic.c
    └── test_card_esp.c
```

---

## 6. 测试（编译前必跑）

```bash
cd esp32/tests && ./run_tests.sh
```

> 注：调试输出经 USB-Serial-JTAG（原生 USB 口），UART0（GPIO43/44）不占用，可留作他用。
- `test_logic.c`：触发词矩阵/计数/去重（复用 stm32 测试，路径指向 esp32 项目）
- `test_card_esp.c`：U13T 帧解析（主机编译，无硬件依赖）

---

## 7. 文档导航

1. `esp32_tts_rtos/docs/01-项目说明.md` —— RTOS 版：硬件映射、架构、移植说明、构建步骤、验收清单
2. `esp32_tts_baremetal/docs/01-项目说明.md` —— 裸机版：同上（单任务超级循环）

功能规格的完整定义（A~I 参数、触发词行为矩阵、停车时序）与 STM32 版相同，以 `stm32/` 下两版 `docs/02-项目说明.md` 为准；本文档只重述要点并聚焦 ESP32 移植差异。

---

## 8. ⚠️ 设计决策（待确认，确认后开始写代码）

| # | 决策项 | 本方案默认值 | 备选 |
|---|---|---|---|
| 1 | 电机驱动方式 | **方案 A**：LEDC 双通道互补（GPIO4/5），`Motor_Control(speed 0~999)` 接口与 STM32 版完全一致，逻辑零改动 | 方案 B：LEDC 单通道 PWM + IN1/IN2 方向 GPIO（需改电机逻辑） |
| 2 | 调试输出口 | ✅ 已选 **USB-Serial-JTAG**（原生 USB 口，免转接；sdkconfig.defaults 配置 console） | ~~UART0 console~~ |
| 3 | 项目名 | `esp32_tts_rtos` / `esp32_tts_baremetal` | 可改 |
| 4 | 引脚分配 | 见第 2 节表格（全部 config.h 可改） | 按你的板子实际引脚调整 |
| 5 | 电机 PWM 频率 | 20kHz（LEDC 常规值，高于 STM32 版 1kHz，更静音；速度语义不变） | 1kHz（与 STM32 版一致） |

确认以上决策后，按 `docs/` 规划实施代码。
