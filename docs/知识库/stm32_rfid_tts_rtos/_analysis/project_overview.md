# 项目概览（导航）
- 芯片: **STM32F103C8T6**（Cortex-M3，64KB Flash / 20KB RAM）
- 版本: RTOS 版（FreeRTOS CMSIS_V1）；构建: CubeMX 6.17 生成 + CMake/GCC（arm-none-eabi 14.2.1）
- 功能: 读卡(U13T) + TTS 播报 + 电机控制（多触发词停车/降速/电位器停止）+ LED + USART3 调试口
- 业务源文件: 17 个；头文件: 186 个；业务函数: 67
- 业务文件清单: Task/motor_control_task.c, Task/rfid_logic.c, Task/rfid_task.c, Task/motor_logic.c, Core/Src/gpio.c, Core/Src/adc.c, Core/Src/usart.c, Core/Src/main.c, Core/Src/freertos.c, Core/Src/tim.c, Core/Src/stm32f1xx_it.c, hardware/ADC/BSP_ADC.c, hardware/DEBUG/Debug.c, hardware/LED/led.c, hardware/pwm/PWM.c, hardware/USART/BSP_USART.c, hardware/rfid_card/Card.c
- 有效宏: USE_HAL_DRIVER, STM32F103xB, DEBUG
