# ESP32-S3 读卡语音播报控制系统（STM32 项目复刻）

本项目是 stm32/ 下两个 STM32 工程的 **ESP32-S3 复刻版**，包含两个并列项目：

| 目录 | 版本 | 对应 STM32 工程 | 说明 |
|---|---|---|---|
| esp32_tts_rtos/ | RTOS 版 | stm32/rfid_tts_rtos | 双 FreeRTOS 任务（读卡任务 + 电机任务） |
| esp32_tts_baremetal/ | 裸机版 | stm32/rfid_tts_baremetal | 单任务超级循环（顺序执行两个状态机） |

基础功能规格与 STM32 版完全一致，并新增 ESP32 特有能力：**WiFi 配网 + 网页参数配置、RGB LED 状态流转、任意卡内容中文显示（GBK⇄UTF-8 转换层）**。

---

## 1. 功能清单

| # | 功能 | 说明 |
|---|---|---|
| 1~5 | 同 STM32 版 | 晚启动缓启动 / 实时读卡+LED 保持 / TTS 播报去重（上电提示音 <I>7）/ 定时降速（**多段减速窗口 ×8**）/ 多触发词停车 |
| 6 | 参数化自动停车 | 总时长网页可配（默认 300s，范围 10~1000s），NVS 掉电保存（原电位器方案已退役） |
| 7 | WiFi 配网模式 | **快速连按 3 次 RST** 进入：RGB LED 蓝色快闪 → 开热点 EV-Car-Setup（WPA2，密码 12345678）→ 浏览器打开 http://192.168.4.1 修改全部参数 → 保存重启生效 |
| 8 | RGB LED 状态流转 | 刷卡🟢绿 → 停车减速🟡黄 → 静止等待🔴红 → 重新缓启动🟠橙 → 正常运行⚪白 |
| 9 | 中文显示层 | 任意卡内 GBK 内容经内置 GB2312 表转 UTF-8 串口输出，不再限于预设触发词 |
| 10 | 系统保护 | 崩溃自动重启（看门狗）；崩溃复位不计入配网手势计数 |

> 配网手势防误触：每次上电 NVS 计数 +1，持续运行 10 秒自动清零；仅正常上电/RST 计数，PANIC/看门狗复位不累计；配置模式空闲超时自动退出重启。

---

## 2. 开发环境

| 项 | 值 |
|---|---|
| ESP-IDF | v6.0.2 |
| 激活脚本 | ~/.espressif/tools/activate_idf_v6.0.2.sh（按本机安装位置调整） |
| 目标芯片 | ESP32-S3（Xtensa，512KB SRAM，双核） |

**每次编译前必须激活环境**：
```bash
source ~/.espressif/tools/activate_idf_v6.0.2.sh    # 路径按本机 IDF 安装调整
cd esp32/esp32_tts_rtos          # 或 esp32_tts_baremetal
idf.py set-target esp32s3        # 首次执行一次
idf.py build
idf.py -p /dev/ttyACM0 flash monitor                 # USB-Serial-JTAG 口
```

> ⚠️ 中文路径说明：IDF 默认 picolibc 不支持中文路径，两版 sdkconfig.defaults 已配置 CONFIG_LIBC_NEWLIB=y 绕开，**请勿删除**。

---

## 3. 硬件接线表（引脚统一在 components/common/pins.h 修改，两版共用）

| 功能 | 引脚 | 外设 | 参数 |
|---|---|---|---|
| 读卡模块 U13T | GPIO10 = TX，GPIO11 = RX | UART1 | 9600，初始化后切 115200 |
| 语音模块 CN-TTS | GPIO12 = TX，GPIO13 = RX | UART2 | 9600，8N1 |
| 电机 PWM 输出 1 / 2 | GPIO4 / GPIO5 | LEDC 通道 0 / 1 | 20kHz，10-bit，差分互补（转向=角色交换） |
| RGB LED | ws2812（见 pins.h） | RMT 驱动 | 状态流转配色见 §1 功能 8 |
| LED 指示灯（兼容） | GPIO2、GPIO8、GPIO9 | GPIO 推挽 | 低电平点亮 |
| USB-Serial-JTAG | 原生 USB 口 | console | idf.py monitor 直接查看，免 USB 转 TTL |

> 电位器（GPIO1/SARADC1）已退役：GPIO1 悬空即可，ADC 采样代码已从电机流程移除（components/common/adc.c 为遗留文件，无调用方）。

接线注意：串口 TX↔RX 交叉；CN-TTS 必须 5V 独立供电（峰值 320mA）；U13T 供电 3.0~5.5V；GPIO10/11/12/13 均 5V 容忍（FT）可直连；共地。UART0（GPIO43/44）不占用，可留他用。

---

## 4. WiFi 配网与网页参数

- 触发：5 秒内连续通断电/按 RST 3 次 → 蓝色快闪 → SoftAP「EV-Car-Setup」（WPA/WPA2，密码 12345678，IP 192.168.4.1）
- 网页（webpage.html 内嵌单文件）：时间字段单位秒（支持一位小数）；电机转向下拉框；减速窗口可增删行（JS 校验不重叠并按时间排序）；触发词直接输汉字、旁实时预览 GBK hex；提交前范围预校验
- API：GET /api/params 回显（触发词转中文）；POST /api/params 保存（UTF-8→GBK 转换 + sanitize + NVS）；POST /api/restart 保存并重启生效
- 参数层 nvs_params：params_t 全量参数（电机时序/转向/减速窗口×8/规则×8/LED/去重/轮询/autostop_ms），NVS 无值回落 config.h 默认宏

---

## 5. 两版实现对比

| 对比项 | esp32_tts_rtos | esp32_tts_baremetal |
|---|---|---|
| 调度 | 两个 FreeRTOS 任务（xTaskCreate） | app_main 单任务 while(1) |
| 读卡/电机流程 | RFID_Task（栈4096B，优先级5）、Motor_Task（栈2048B，优先级1） | RFID_Process() / Motor_Process() 主循环调用 |
| 延时 | vTaskDelay 可阻塞 | esp_timer 时间差非阻塞 |
| 实时性 | TTS 发送只卡自己任务 | TTS 发送期间主循环毫秒级停顿（可接受） |

> 注：ESP-IDF 内建 FreeRTOS，「裸机版」同样运行在调度器上；两版差异仅在代码组织方式。两版 main 仅允许 4 组差异文件（app_main + rfid/motor 任务或进程版），漂移由 run_tests.sh 检查。

---

## 6. 与 STM32 版的代码复用关系

| 模块 | 说明 |
|---|---|
| config.h | 宏保留作**默认值来源**（nvs_params 无值时回落）；运行时一切读 g_params |
| rfid_logic.c / motor_logic.c | 与 STM32 版逐字相同（Setter 注入式纯逻辑，四版共享，主机可测） |
| card_parse.c | U13T 帧解析独立纯逻辑（含长度上限防护、仅 0x91 产生事件），ESP32 特有组件 |
| gbk_utf8.c(+table) | GB2312⇄UTF-8 映射表（约15KB const 入 Flash），网页回显与串口中文显示共用 |
| nvs_params / param 体系 | STM32 版已做**同构实现**（存储介质不同：Flash 末页 vs NVS） |
| tts.c | uart_write_bytes(UART2) 直发；上电提示音 <I>7（试听选定） |
| 数据输出协议 | [SYS]/[RFID]/[LED]/[MOTOR] 格式与 STM32 版一致 |

---

## 7. 目录结构

```text
esp32/
├── README.md                        # 本文件
├── docs/WiFi配置模式实施计划.md      # 配网/参数层/网页 设计与验收清单
├── components/common/               # ★ 共享业务组件（两版唯一维护点）
│   ├── config.h / pins.h            # 默认参数宏 / 引脚与外设实例映射
│   ├── rfid_logic / motor_logic     # 纯逻辑层（四版逐字共享）
│   ├── card_uart / card_parse       # U13T 协议驱动 / 帧解析纯逻辑
│   ├── tts / motor_drv / led / debug# TTS发送 / LEDC电机 / LED / 调试
│   ├── gbk_utf8 (+_table)           # GB2312⇄UTF-8 转换
│   ├── nvs_params                   # 运行时参数层（NVS）
│   ├── config_mode / wifi_ap / web_server / webpage.html   # 配网三件套+页面
│   ├── ws2812                       # RGB LED（RMT）
│   └── adc                          # （遗留，无调用方）
├── esp32_tts_rtos/main/             # app_main + rfid_task + motor_task
├── esp32_tts_baremetal/main/        # app_main + rfid_process + motor_process
└── tests/                           # 主机单元测试 run_tests.sh
```

---

## 8. 主机单元测试（编译前必跑）

```bash
cd esp32/tests && ./run_tests.sh
```

当前 **87 项全绿 + 漂移检查 OK**：rfid_logic 32 + card_parse 14 + motor_logic 30 + gbk_utf8 11。

---

## 9. 文档导航

| 文档 | 内容 |
|---|---|
| esp32_tts_rtos/docs/01-项目说明.md（裸机版同名） | 硬件映射、架构、构建步骤、验收清单 |
| esp32/docs/WiFi配置模式实施计划.md | 配网手势/参数层/网页 API 设计与集成验收清单 |
| stm32/docs/02-项目说明.md | 功能规格权威定义（A~I 参数、触发词行为矩阵、停车时序） |
| stm32/docs/串口配置模式实施计划.md | STM32 参数化对齐方案（CLI/上位机/ISP） |
| docs/知识库/esp32_tts_rtos（及另三项目） | 知识库 00~11 + SUMMARY + 验收包 |

---

## 10. 设计决策（已确认并实施）

| # | 决策项 | 已实施 |
|---|---|---|
| 1 | 电机驱动 | LEDC 双通道互补（GPIO4/5），20kHz 10-bit，支持正反转 |
| 2 | 调试输出 | USB-Serial-JTAG console（sdkconfig.defaults 配置） |
| 3 | 项目名 | esp32_tts_rtos / esp32_tts_baremetal |
| 4 | 配置入口 | 连按 3 次 RST（等效快速通断电），SoftAP + HTTP 网页 |
| 5 | 参数化 | nvs_params 运行时层，config.h 宏转为默认值；自动停车网页化（电位器退役） |
| 6 | 多段减速 | slowwins[8] 列表，SLOW 仅从 RUN 进入，STOPPING/WAIT/RAMPUP 优先级更高 |
