# 中断/调度（ESP-IDF FreeRTOS）
- UART/RMT/NVS 驱动中断由 IDF 内部管理（环形缓冲/队列），应用层轮询消费，无显式优先级配置
- 任务优先级（RTOS 版）: RFID_Task=5（栈4096）、Motor_Task=1（栈2048）、cfg_clear_task=1（栈4096，仅配置模式检测后临时）
- FreeRTOS tick **1000Hz**（CONFIG_FREERTOS_HZ=1000）：vTaskDelay(1)=1ms；超时类逻辑另用 esp_timer 真实时间差
- 注意：main/rfid_task.c:36 注释仍写"默认 100Hz"，已过时，以 sdkconfig 为准