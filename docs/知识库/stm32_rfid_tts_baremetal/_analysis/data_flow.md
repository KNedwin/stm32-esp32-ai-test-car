# 数据流（导航）
## 读卡链路
USART1 RX 中断 → HAL_UART_RxCpltCallback（Card.c）→ 帧解析 UartReceiveCommand → card_res_flag
→ RFID 状态机（EXIST→WAIT→RESDATA→LEDLIGHT→NONE）→ chinese_data[16]
→ RfidLogic_Process（触发匹配/计数/去重，规则来自 s_rules←params_apply）→ 事件 → TTS 播报（USART2）/ motor_trigger_flag
## 电机链路（2026-08：电位器退役）
g_params.autostop_ms → Motor_Init(stop_time) → MotorLogic_Step（绝对计时状态机，trigger 消费）→ Motor_Control(dir) → TIM2 CH1/CH2 PWM
## 触发停车链路
RfidLogic 返回 EV_TRIGGER_STOP → motor_trigger_flag=1 → MotorLogic_Step 消费 → STOPPING→WAIT→RAMPUP
## 参数配置链路（2026-08 新增）
PC 上位机/串口终端 → USART3 RXNE 轮询(ParamCli_Poll) → param_cli_execute 行解析 → g_params 修改 → params_apply() 注入逻辑层即时生效 → SAVE 时 params_save() 写 Flash 0x0800FC00
## 启动参数链路
Flash 末页(magic+CRC 校验过) → params_init() 读入 g_params（失败回落 config.h 默认）→ params_apply()
