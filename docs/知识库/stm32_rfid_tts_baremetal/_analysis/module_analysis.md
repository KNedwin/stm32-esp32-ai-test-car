# 模块分析（导航，19 个业务 .c）
| 模块 | 文件 | 职责 | 核心函数 |
|---|---|---|---|
| 读卡协议 | hardware/rfid_card/Card.c|h | U13T 帧收发/解析、波特率切换、接收回调、ORE 自愈 | SetBound115200/ReadCard/ReadBlock/UartReceiveCommand/HAL_UART_RxCpltCallback/HAL_UART_ErrorCallback |
| 触发/去重逻辑 | Task/rfid_logic.c|h | 触发词匹配/计数/去重决策（纯逻辑，四版共享；Setter 注入） | RfidLogic_SetConfig/RuleCount/Process/TriggerMatch/IsDup/UpdateSpeak |
| 电机状态机 | Task/motor_logic.c|h | 电机时序纯逻辑（四版共享；多段降速窗口；Setter 注入） | MotorLogic_SetTiming/Init/Step/IsInStopSequence/CalcStopTime |
| 读卡状态机 | Task/rfid_process.c|h | 五态读卡流程+播报+LED（LED 延时/轮询间隔改用 g_params） | RFID_Init(含<I>7)/RFID_Process |
| 电机应用 | Task/motor_process.c|h | 转向参数+喂 motor_logic+输出 PWM（无 ADC 采样） | Motor_Init/Motor_Process/Motor_IsInStopSequence |
| 参数存储 | config/nvs_params.c|h | params_t 同构参数存 Flash 末页+CRC16MODBUS；sanitize 钳制；apply 注入逻辑层 | params_init/save/apply/sanitize/crc16 |
| 串口配置 CLI | config/param_cli.c|h | USART3 行协议 HELP/GET/SET/SAVE/DUMP/ISP/REBOOT；每条修改后即时 apply | ParamCli_Init/Poll/param_cli_execute/set_scalar/hex_to_bytes/respond |
| ISP 软跳 | config/isp_jump.c|h | 软跳系统 Bootloader 0x1FFFF000 + REBOOT 复位钩子 | ISP_Enter/param_cli_do_isp/do_reboot |
| TTS 发送 | hardware/USART/BSP_USART.c|h | printf 重定向(__io_putchar→USART2)、字符串发送 | Usartx_SendString |
| 电机驱动 | hardware/pwm/PWM.c|h | Motor_Control(0~999) 双路 PWM2 + **Motor_SetDirection 正反转** | Motor_SetDirection/Motor_Control/PWM_Init/PWM_DutySet |
| LED | hardware/LED/led.c|h | 三引脚灯控（边沿调试输出） | LED_Sta |
| 电位器(退役) | hardware/ADC/BSP_ADC.c|h | ADC 采样——已无调用方，留编译 | Get_ADC_Value |
| 调试输出 | hardware/DEBUG/Debug.c|h | USART3 数据口（CLI 应答也走此口） | Dbg_Init/Dbg_Printf |
