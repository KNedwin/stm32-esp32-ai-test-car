#ifndef _CARD_H
#define _CARD_H

#include "stm32f1xx_hal.h"

/* 读卡模块串口（USART1） */
#define CARD_HAL_USARTx					huart1
#define CARD_USARTx						USART1

/* 读卡流程标志 */
#define CARD_FLAG_NONE					0   /* 无卡，等待 */
#define CARD_FLAG_RESDATA  				1   /* 已收到完整数据（读块） */
#define CARD_FLAG_WAIT					2   /* 已发命令，等待模块响应 */
#define CARD_FLAG_EXIST					3   /* 检测到卡，准备读块 */
#define CARD_FLAG_LEDLIGHT				4   /* LED 亮灯保持中 */

typedef struct _CMD
{
	unsigned char ReceiveBuffer[32];
	unsigned char SendBuffer[32];
	unsigned char ReceivePoint;
	unsigned char Code;
	unsigned char block_data[16];
}CMD;

typedef struct _CARD
{
	unsigned int Type;
	unsigned long UID;
	unsigned long Value;
	unsigned char KeyA[6];
	unsigned char KeyB[6];
	unsigned char BlockData[16];
}CARD;

extern CARD Card;
extern CMD Cmd;

/* 卡在场时间戳（ms，由接收中断回调刷新；rfid 状态机据此判断 LED 熄灭） */
extern volatile uint32_t rfid_last_card_tick;

void SetBound115200(void);
void SetBound9600(void);
void ReadCard(void);
void ReadBlock(unsigned char block);
void WriteBlock(unsigned char block, unsigned char *blockData);
void MakeCard(unsigned char block, unsigned long value);
void Inc(unsigned char block, unsigned long value);
void Dec(unsigned char block, unsigned long value);
void ClearCard(unsigned char block);
uint8_t UartReceiveCommand(uint8_t data);

#endif
