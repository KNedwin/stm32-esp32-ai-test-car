# 启动顺序与任务
## 启动顺序（app_main，2026-08-25 现状）
LED_Init → Motor_Drv_Init → Motor_Control(0) → Dbg_Init → Card_Uart_Init（含波特率切换）→ TTS_Init
→ params_init（NVS 加载 g_params，必须先于 config_mode 检测）→ Motor_SetDirection(g_params.motor_dir)
→ config_mode_boot_check → [命中则 config_mode_run：SoftAP+HTTP 永不返回]
→ MotorLogic_SetTiming / RfidLogic_SetConfig（g_params 注入）→ xTaskCreatePinnedToCore(RFID_Task pri5 / Motor_Task pri1)
- ~~ADC_Init~~ 已移除（电位器退役）；~~Motor_Init 电位器采样~~ 改 Motor_Task 内 MotorLogic_Init(stop_time=g_params.autostop_ms)

## 任务（RTOS 版）
| 任务 | 优先级 | 栈 | 职责 |
|---|---|---|---|
| RFID_Task | 5 | 4096B | 读卡五态/播报/LED/触发（rfid_task.c；任务内先 TTS_SetupDefaults） |
| Motor_Task | 1 | 2048B | 喂 motor_logic + PWM 输出 + RGB 状态色（motor_task.c） |
| cfg_clear_task | 1 | 4096B | 上电 10s 后清 NVS cfg_cnt 计数（config_mode.c，非配置模式时也创建） |