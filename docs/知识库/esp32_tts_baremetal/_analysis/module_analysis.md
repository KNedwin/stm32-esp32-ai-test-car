# 模块分析（导航，18 业务 .c 全覆盖）
## 应用层（main/，裸机版差异文件）
| 模块 | 文件 | 职责 | 导出函数 |
|---|---|---|---|
| 入口 | app_main.c | 初始化序列 + while(1) 超级循环 + 参数/规则注入 | app_main |
| 读卡应用 | rfid_process.c|h | 五态状态机+绿色确认窗+播报/触发执行 | RFID_Init / RFID_Process |
| 电机应用 | motor_process.c|h | autostop 参数喂逻辑层+速度输出+状态边沿 RGB | Motor_Init / Motor_Process / Motor_IsInStopSequence / Motor_IsBusyForLed |

## 共享组件（components/common/，两版唯一维护点）
| 模块 | 文件 | 职责 | 导出函数 |
|---|---|---|---|
| 读卡驱动 | card_uart.c|h | UART1 驱动+组帧转义+波特率切换（DEMO_MODE 模拟器内嵌） | Card_Uart_Init/ReadCard/ReadBlock/Poll |
| 帧解析 | card_parse.c|h | U13T 帧解析纯逻辑+状态标志 | Card_Parse_Feed / UartReceiveCommand |
| 触发/去重逻辑 | rfid_logic.c|h | 触发词匹配/计数/去重（Setter 注入规则） | RfidLogic_SetConfig/RuleCount/Init/TriggerMatch/IsDup/UpdateSpeak/Process |
| 电机状态机 | motor_logic.c|h | 绝对计时状态机（Setter 注入时序，slowwin×8） | MotorLogic_SetTiming/Init/Step/IsInStopSequence/CalcStopTime/StateName |
| 电机驱动 | motor_drv.c|h | LEDC 双通道互补+方向切换 | Motor_Drv_Init / Motor_Control / Motor_SetDirection |
| TTS | tts.c|h | UART2 发送+开机默认指令(<S>3/<V>6/<I>7) | TTS_Init / TTS_SetupDefaults / TTS_Send |
| LED | led.c|h | 三引脚灯+WS2812 颜色联动 | LED_Init / LED_Sta / LED_SetColor |
| WS2812 | ws2812.c|h | RMT simple encoder 驱动 GRB | WS2812_Init / WS2812_SetColor |
| ADC（退役） | adc.c|h | adc_oneshot 采样——无调用方，留编译 | ADC_Init / Get_ADC_Value |
| 调试 | debug.c|h | printf console 封装 | Dbg_Init / Dbg_Printf |
| 参数层 | nvs_params.c|h | 默认值表/sanitize 钳制/NVS blob 存取 | params_init / params_sanitize / params_save |
| GBK⇄UTF-8 | gbk_utf8.c|h(+table) | GB2312⇄UTF-8 双向查表（8178 项） | gbk_to_utf8 / utf8_to_gbk |
| 配网检测 | config_mode.c|h | 连按 RST NVS 计数+进入判定+配网宿主循环 | config_mode_boot_check/should_enter/run |
| SoftAP | wifi_ap.c|h | EV-Car-Setup 热点（WPA2，12345678） | wifi_ap_start |
| Web 服务 | web_server.c|h | HTTP 页面+参数 JSON API+重启 | web_server_start / web_server_idle_seconds |
（2026-08 更新：模块从 10 个扩到 18 个；函数清单按当前头文件逐一核对）