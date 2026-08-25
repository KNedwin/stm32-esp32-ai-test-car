#ifndef __ISP_JUMP_H
#define __ISP_JUMP_H

/* 软跳转到芯片出厂系统 Bootloader（USART1 ISP，地址 0x1FFFF000）
 * 用于免 ST-Link 免 BOOT0 跳线的现场串口烧录。
 * 注意：跳转前需拔掉 U13T 读卡模块（其 RX 占用 USART1 会干扰应答）。 */
void ISP_Enter(void);

#endif /* __ISP_JUMP_H */
