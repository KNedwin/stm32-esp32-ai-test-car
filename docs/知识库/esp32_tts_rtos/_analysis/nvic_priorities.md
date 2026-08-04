# 中断/调度（ESP-IDF FreeRTOS）
- USART 驱动: 中断内部处理（环形缓冲），应用层轮询消费，无显式优先级配置
- 任务优先级（RTOS 版）: RFID_Task=5，Motor_Task=1
- FreeRTOS 节拍 100Hz，vTaskDelay(1)=10ms
