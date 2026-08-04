# 内存布局
- Flash: 2MB（分区 factory 1MB，固件约 237KB）
- SRAM: 512KB（RTOS 版 bss≈11.7KB 量级，充裕）
- 栈: RFID_Task 4096B / Motor_Task 2048B（RTOS 版）；main 任务默认 3584B（裸机版）
