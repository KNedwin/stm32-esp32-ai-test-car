# 内存布局
- Flash: 2MB（sdkconfig:CONFIG_ESPTOOLPY_FLASHSIZE_2MB=y）；固件内嵌资源: webpage.html 约 7.8KB（EMBED_FILES）、gbk_utf8_table 映射表 8178 项 uint16 约 16KB（Flash 常量）
- SRAM: 512KB（业务 bss 十余 KB 量级，充裕）
- 栈: main 任务 3584B（CONFIG_ESP_MAIN_TASK_STACK_SIZE）；cfgclr 任务 4096B（NVS 写入峰值 >1.5KB 的经验值）
- NVS 分区: 参数 blob(sizeof(params_t)) + cfg_cnt(u8)
（2026-08 更新：裸机版任务栈与内嵌资源现状；RTOS 版 4096/2048B 栈描述仅适用于 rtos 版）