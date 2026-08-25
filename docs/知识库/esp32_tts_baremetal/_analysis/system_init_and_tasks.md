# 启动顺序与任务
## 启动顺序（main/app_main.c app_main，2026-08 核实）
LED_Init → Motor_Drv_Init → Motor_Control(0) → Dbg_Init([SYS] boot) → Card_Uart_Init（含 0x2C 波特率切换）
→ TTS_Init → **params_init**（NVS 初始化+读参数 blob+sanitize）→ **Motor_SetDirection(g_params.motor_dir)**
→ **config_mode_boot_check**（NVS 连按计数；达阈值则 config_mode_run 进入配网，永不返回直至 esp_restart）
→ MotorLogic_SetTiming + RfidLogic_SetConfig（g_params 注入逻辑层，static 存储保证指针持久）
→ Motor_Init（**g_params.autostop_ms 直通 MotorLogic_Init，无 ADC 采样**）
→ RFID_Init（延时 500ms + TTS_SetupDefaults，阻塞约 0.9s）
→ while(1){ RFID_Process; Motor_Process; vTaskDelay(pdMS_TO_TICKS(1)) }

## 任务（裸机版）
| 任务 | 优先级 | 栈 | 职责 |
|---|---|---|---|
| main(app_main) | 1(默认) | 3584B | 超级循环：RFID_Process + Motor_Process |
| cfgclr | 1 | 4096B | 上电 10s 后清 NVS 配网计数（config_mode.c cfg_clear_task） |

（2026-08 更新：启动序列加入参数加载/转向应用/配网检测/逻辑层注入；移除 ADC_Init；任务表现状）