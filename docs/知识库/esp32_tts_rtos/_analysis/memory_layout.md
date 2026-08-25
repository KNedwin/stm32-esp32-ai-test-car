# 内存布局
- Flash: 模组物理 16MB（N16R8）；工程 sdkconfig 按 **2MB** 布局（CONFIG_ESPTOOLPY_FLASHSIZE="2MB"）+ partitions_singleapp.csv
- 固件 build/esp32_tts_rtos.bin ≈ 924KB（含填充）
- SRAM: 512KB，充裕；NVS blob params_t ≈ 数百字节存 Flash NVS 分区
- 栈: RFID_Task 4096B / Motor_Task 2048B / cfg_clear_task 4096B（NVS 写调用链峰值 >1.5KB 的教训注释在 config_mode.c）；main 任务默认 3584B