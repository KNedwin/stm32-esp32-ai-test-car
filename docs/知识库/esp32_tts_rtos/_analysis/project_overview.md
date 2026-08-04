# 项目概览（导航）
- 芯片: **ESP32-S3**（Xtensa LX7 双核，240MHz，512KB SRAM）
- 版本: RTOS 版（双任务）；构建: ESP-IDF v6.0.2（idf.py，newlib，USB-Serial-JTAG console）
- 功能: 读卡(U13T) + TTS 播报 + 电机控制（多触发词停车/降速/电位器停止）+ LED + 调试输出
- 业务源文件: 12 个；头文件: 13 个；业务函数: 11
- 组件: components/common 共享（config/pins/驱动/纯逻辑），main 仅应用差异文件
- 有效宏: ESP_PLATFORM, IDF_VER="v6.0.2"
