# 模块分析（导航）
| 模块 | 文件 | 职责 | 核心函数 |
|---|---|---|---|
| 配置参数 | components/common/nvs_params.c|h | params_t 默认表/sanitize 钳制/NVS blob 存取 | params_init/sanitize/save |
| 配网入口 | components/common/config_mode.c|h | 连按3次RST 计数(NVS)→进入 SoftAP+HTTP；10s 清计数 | config_mode_boot_check/run |
| WiFi AP | components/common/wifi_ap.c|h | SoftAP EV-Car-Setup WPA2 12345678 | wifi_ap_start |
| Web 服务 | components/common/web_server.c|h | 页面+参数 JSON GET/POST→GBK 转换→NVS→重启 | web_server_start/idle_seconds |
| 编码转换 | components/common/gbk_utf8.c(+table.h) | GB2312⇄UTF-8 双向（8178 项表） | gbk_to_utf8/utf8_to_gbk |
| 读卡驱动 | components/common/card_uart.c|h | UART1 驱动+命令组帧+波特率切换（DEMO_MODE 模拟器） | Card_Uart_Init/ReadCard/ReadBlock/Poll |
| 帧解析 | components/common/card_parse.c|h | U13T 帧解析纯逻辑 | UartReceiveCommand/Card_Parse_Feed |
| 触发/去重逻辑 | components/common/rfid_logic.c|h | 触发词匹配/计数/去重（四版共享，Setter 注入） | RfidLogic_Process/SetConfig |
| 电机状态机 | components/common/motor_logic.c|h | 电机时序纯逻辑（多段减速窗口，Setter 注入） | MotorLogic_Step/SetTiming/CalcStopTime(共享保留) |
| 电机驱动 | components/common/motor_drv.c|h | LEDC 双通道互补+方向 | Motor_Control/Motor_SetDirection |
| TTS | components/common/tts.c|h | 发送+默认设置(<S>3/<V>6/<I>7) | TTS_Send/SetupDefaults/Init |
| LED/RGB | components/common/led.c ws2812.c | 三引脚灯控 + WS2812B RMT 驱动 | LED_Sta/LED_SetColor/WS2812_SetColor |
| ADC（退役） | components/common/adc.c | 电位器采样（无调用方仍编译） | Get_ADC_Value |
| DEBUG | components/common/debug.c | console 输出 | Dbg_Printf |
| 读卡应用 | main/rfid_task.c|h | 五态状态机+播报+绿色确认窗 | RFID_Task/RFID_HandleCardData |
| 电机应用 | main/motor_task.c|h | 喂 motor_logic+RGB 映射（stop_time=g_params.autostop_ms） | Motor_Task/Motor_IsBusyForLed |
| 入口 | main/app_main.c | 初始化+参数注入+调度 | app_main |