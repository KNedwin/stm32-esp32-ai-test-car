# 模块分析（导航）
| 模块 | 文件 | 职责 | 核心函数 |
|---|---|---|---|
| 参数存储 | config/nvs_params.c\|h | params_t 同构参数、Flash 末页读写+CRC16MODBUS、范围钳制、注入逻辑层 | params_init/params_save/params_apply/params_sanitize |
| 串口配置 CLI | config/param_cli.c\|h | USART3 行协议解析（HELP/GET/SET/SAVE/DUMP/ISP/REBOOT），改后即时 params_apply | param_cli_execute/ParamCli_Poll/ParamCli_Init/ParamCli_ShouldEnterIsp |
| ISP 软跳 | config/isp_jump.c\|h | 软跳系统 Bootloader 0x1FFFF000；REBOOT 复位钩子 | ISP_Enter/param_cli_do_isp/param_cli_do_reboot |
| 读卡协议 | hardware/rfid_card/Card.c\|h | U13T 帧收发/解析、波特率切换、接收回调 | SetBound115200/ReadCard/ReadBlock/UartReceiveCommand/HAL_UART_RxCpltCallback/HAL_UART_ErrorCallback |
| 触发/去重逻辑 | Task/rfid_logic.c\|h | 触发词匹配/计数/去重决策（纯逻辑，四版共享）；SetConfig 运行时注入 | RfidLogic_Process/TriggerMatch/IsDup/UpdateSpeak/RfidLogic_SetConfig/RfidLogic_RuleCount/RfidLogic_Init |
| 电机状态机逻辑 | Task/motor_logic.c\|h | 绝对计时电机时序纯逻辑（四版共享）；SetTiming 运行时注入 | MotorLogic_Step/Init/CalcStopTime/IsInStopSequence/StateName/MotorLogic_SetTiming |
| 读卡任务 | Task/rfid_task.c\|h | 五态读卡流程+播报+LED+触发标志（RTOS 版）；轮询间隔/熄灯延时用 g_params | RFID_Task/RFID_HandleCardData/RFID_Speak/RFID_Par_Init/BufClear |
| 电机应用 | Task/motor_control_task.c\|h | Motor_SetDirection(g_params.motor_dir)+MotorLogic_Init(g_params.autostop_ms)+喂 Step+输出 PWM | Motor_Control_Task/Motor_Init/Motor_ApplySpeed/Motor_IsInStopSequence |
| TTS 发送 | hardware/USART/BSP_USART.c\|h | printf 重定向(__io_putchar→USART2)、字符串发送 | __io_putchar/Usartx_SendString |
| 电机驱动 | hardware/pwm/PWM.c\|h | Motor_Control(0~999) 双路 PWM2 差分；Motor_SetDirection 正反转 | PWM_Init/Motor_Control/Motor_SetDirection/PWM_DutySet |
| LED | hardware/LED/led.c\|h | 三引脚灯控（边沿调试输出） | LED_Sta |
| 电位器采样 | hardware/ADC/BSP_ADC.c\|h | **已退役**：Get_ADC_Value 无调用方（留编译） | Get_ADC_Value |
| 调试输出 | hardware/DEBUG/Debug.c\|h | USART3 数据口（CLI 应答同走此口） | Dbg_Init/Dbg_Printf |