# 项目概览（导航）
- 芯片: **ESP32-S3**（Xtensa LX7 双核；sdkconfig 设定 CPU **160MHz**: CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ_160=y；XTAL 40MHz；512KB SRAM）
- 版本: 裸机版（app_main 单任务超级循环 + 配置模式辅助任务 cfgclr）；构建: ESP-IDF v6.0.2（idf.py，newlib，USB-Serial-JTAG console，FreeRTOS tick 1000Hz）
- 功能: 读卡(U13T) + TTS 播报 + 电机控制（多触发词停车/多段降速/网页设定自动永久停止）+ 三引脚 LED 与板载 WS2812 RGB 状态灯 + 调试输出 + WiFi 配网参数配置
- 业务源文件: **18 个 .c**（components/common 15 + main 3）；头文件 20 个；导出函数约 52 个
- 组件: components/common 共享（config/pins/驱动/纯逻辑/参数层/配网四件套），main 仅应用差异文件（app_main + rfid_process + motor_process）
- 托管依赖: espressif/cjson ^1.7.3（idf_component.yml；IDF v6 移除内置 json 组件后改托管，dependencies.lock 锁 1.7.19~2）
- 有效宏: ESP_PLATFORM、IDF_VER="v6.0.2"、DEMO_MODE=0（演示模拟器代码仍保留于 card_uart.c/tts.c，编译期裁剪）
（2026-08 更新：源码统计、CPU 频率、cJSON 依赖、DEMO_MODE 现状均按当前源码核实）