# 项目概览（导航）
- 芯片: **STM32F103C8T6**（Cortex-M3，64KB Flash / 20KB RAM）
- 版本: 裸机版（无 RTOS，主循环+状态机）；构建: CubeMX 6.17 生成 + CMake/GCC（arm-none-eabi 14.2.1）
- 功能: 读卡(U13T) + TTS 播报 + 电机控制（多触发词停车/多段降速/自动停车）+ LED + USART3 调试/串口配置模式
- 业务源文件: 19 个；头文件: ~150 个；业务函数: 77（call_graph_hint.json 85 函数扣除 it.c 8 个标准异常处理器）
- 业务文件清单: Task/rfid_process.c, Task/rfid_logic.c, Task/motor_logic.c, Task/motor_process.c, Core/Src/gpio.c, Core/Src/adc.c, Core/Src/usart.c, Core/Src/main.c, Core/Src/tim.c, Core/Src/stm32f1xx_it.c, hardware/ADC/BSP_ADC.c, hardware/DEBUG/Debug.c, hardware/LED/led.c, hardware/pwm/PWM.c, hardware/USART/BSP_USART.c, hardware/rfid_card/Card.c, **config/nvs_params.c, config/param_cli.c, config/isp_jump.c**（后三者为 2026-08 新增）
- 配套工具（仓库根 tools/stm32_host/，非固件源码）: host.py + page.html —— PC 上位机（HTTP 127.0.0.1:8321 ⇄ USART3 CLI 桥 + 编译/烧录）
- 有效宏: USE_HAL_DRIVER, STM32F103xB, DEBUG
