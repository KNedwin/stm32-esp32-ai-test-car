# 项目概览（导航）
- 芯片: **STM32F103C8T6**（Cortex-M3，64KB Flash / 20KB RAM）
- 版本: RTOS 版（FreeRTOS CMSIS_V1，HAL 时基 TIM1）；构建: CubeMX 6.17 生成 + CMake/GCC
- 功能: 读卡(U13T) + TTS 播报 + 电机控制（多触发词停车/降速/参数化自动停车）+ LED + USART3 调试/配置口
- 新增（2026-08，e992be6/f7870c5/5c22457）: 参数化底座（Setter 注入 + Flash 末页存储）、USART3 串口配置模式（param_cli 行协议）、ISP 软跳烧录（isp_jump）、PC 上位机（仓库根 tools/stm32_host/）
- 业务源文件: 20 个；业务函数: 约 90（含新增 config/ 三模块）
- 业务文件清单: Task/{motor_control_task,rfid_logic,rfid_task,motor_logic}.c，Core/Src/{gpio,adc,usart,main,freertos,tim,stm32f1xx_it}.c，hardware/{ADC/BSP_ADC,DEBUG/Debug,LED/led,pwm/PWM,USART/BSP_USART,rfid_card/Card}.c，config/{nvs_params,param_cli,isp_jump}.c
- 有效宏: USE_HAL_DRIVER, STM32F103xB, DEBUG
- 单测: stm32/tests/run_tests.sh = Card 25 + rfid_logic 32 + motor_logic 30 + 仿真 22 = 109 项全绿
