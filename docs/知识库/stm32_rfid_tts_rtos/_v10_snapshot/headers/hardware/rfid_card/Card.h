#ifndef _CARD_H
#define _CARD_H

#include "stm32f1xx_hal.h"

/* 读卡模块串口（USART1） */
#define CARD_HAL_USARTx					huart1
#define CARD_USARTx						USART1

/* 读卡流程标志（由 rfid 状态机维护，ISR 回调置位） */
#define CARD_FLAG_NONE					0   /* 无卡，等待 */
#define CARD_FLAG_RESDATA  				1   /* 已收到完整数据（读块） */
#define CARD_FLAG_WAIT					2   /* 已发命令，等待模块响应 */
#define CARD_FLAG_EXIST					3   /* 检测到卡，准备读块 */
#define CARD_FLAG_LEDLIGHT				4   /* LED 亮灯保持中 */

typedef struct _CMD
{
	unsigned char ReceiveBuffer[32];
	unsigned char SendBuffer[32];
	unsigned char block_data[16];
}CMD;

extern CMD Cmd;

extern uint8_t card_res;
extern volatile uint8_t card_res_flag;

/* 卡在场时间戳（ms，由接收中断回调刷新；rfid 状态机据此判断 LED 熄灭） */
extern volatile uint32_t rfid_last_card_tick;

void SetBound115200(void);
void ReadCard(void);
void ReadBlock(unsigned char block);
uint8_t UartReceiveCommand(uint8_t data);

#endif
