# 内存布局
- Flash: 0x08000000，64KB（STM32F103XX_FLASH.ld）；**末页 0x0800FC00~0x0800FFFF（1KB）保留参数存储**（config/nvs_params.c:PARAMS_PAGE_ADDR），代码区实际可用至 63KB
- RAM: 0x20000000，20KB；RTOS 版 bss≈11.7KB（含 FreeRTOS 堆 configTOTAL_HEAP_SIZE=8192B）
- 栈: MSP 0x400；FreeRTOS 堆内任务栈: RFID 1024 字 + defaultTask 512 字 + MOTOR 512 字 + Idle 128 字
- flash_blob_t = magic(2)+crc(2)+params_t，远小于 1KB 页