# 内存布局
- Flash: 0x08000000，64KB（.ld: STM32F103XX_FLASH.ld）；裸机版链接产物约 28~30KB
- **参数页**: 0x0800FC00（Flash 最后一页 1KB，64KB 版专用；flash_blob_t=magic2+crc2+params_t），config/nvs_params.c
- RAM: 0x20000000，20KB；裸机版 bss≈3KB（新增 g_params(params_t)+line_buf[192] 增幅 <0.5KB）
- 栈: main 0x400（startup_stm32f103xb.s）
