# 项目概览（导航）
- 芯片: **ESP32-S3**（Xtensa LX7 双核，240MHz，512KB SRAM；模组 ESP32-S3-DevKitC-1 N16R8）
- 版本: RTOS 版（双任务）；构建: ESP-IDF v6.0.2（idf.py，newlib，USB-Serial-JTAG console）
- 功能: 读卡(U13T) + TTS 播报 + 电机控制（多触发词停车/多段降速/网页设定自动停车）+ LED(三引脚+WS2812 RGB) + WiFi 配网参数层 + 调试输出
- 业务源文件: 18 个 .c（common 15 + main 3）；头文件: 19 个；业务函数: ~70
- 组件: components/common 共享（config/pins + 驱动层 + 纯逻辑层 + 参数/配网），main 仅应用差异文件
- 有效宏: ESP_PLATFORM, IDF_VER="v6.0.2"；sdkconfig.defaults: FREERTOS_HZ=1000 / LIBC_NEWLIB / USB_SERIAL_JTAG console
- （2026-08-25 刷新：自动停车网页化 d8c559b、<I>7 1ba84fc、LED/轮询参数化 40b8a47）