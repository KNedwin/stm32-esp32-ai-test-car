# 中断/调度（ESP-IDF FreeRTOS，tick 1000Hz）
- USART/RMT 驱动: 中断在驱动内部处理（环形缓冲/TX 队列），应用层轮询消费，无显式优先级配置
- 裸机版: app_main 单任务超级循环，vTaskDelay(pdMS_TO_TICKS(1))=**1ms**（CONFIG_FREERTOS_HZ=1000）
- 辅助任务: cfg_clear_task（config_mode_boot_check 创建，优先级 1，栈 4096B——NVS 写入调用链峰值 >1.5KB，栈小会溢出 panic）
- main 任务栈: CONFIG_ESP_MAIN_TASK_STACK_SIZE=3584（sdkconfig）
（2026-08 更新：节拍 1000Hz、cfgclr 任务与栈参数按源码核实）