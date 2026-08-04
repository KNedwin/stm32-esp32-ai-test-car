#ifndef __CARD_UART_H
#define __CARD_UART_H

#include <stdint.h>

/* UART1 初始化 + 波特率切换（9600 发命令 → 115200） */
void Card_Uart_Init(void);

/* 读卡号命令（0x10） */
void Card_ReadCard(void);

/* 读块数据命令（0x11） */
void Card_ReadBlock(uint8_t block);

/* 非阻塞消费 UART1 接收缓冲，逐字节喂帧解析器（帧解析在 card_parse.h） */
void Card_Uart_Poll(void);

#endif
