# 内存布局
- Flash: 0x08000000，64KB（.ld: STM32F103XX_FLASH.ld，链接产物约 34KB（RTOS）/28KB（裸机））
- RAM: 0x20000000，20KB；RTOS 版 bss≈11.7KB（含 FreeRTOS 堆 8192B），裸机版 bss≈2.5KB
- 栈: main 0x400；FreeRTOS 堆 configTOTAL_HEAP_SIZE=8192（RTOS 版）
