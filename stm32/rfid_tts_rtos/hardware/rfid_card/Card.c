/******************** (C) COPYRIGHT 2015 ********************
* 文件名          : Card.c
* 描述            : U13T 读卡模块命令与协议解析（案例源码移植+改造）
* 改造点          : ① temp_res==2（读到卡号）时在 LEDLIGHT 态刷新全局卡在场
*                    时间戳 rfid_last_card_tick（供 LED 延迟熄灭判断）
*                  ② temp_res==3（无卡）分支不再递减 led_close_counts
*                    主动熄灯，熄灭改由 rfid 状态机按 tick 判断
********************************************************************************/
#include "Card.h"

CARD Card;
CMD Cmd;
extern UART_HandleTypeDef CARD_HAL_USARTx;
uint8_t card_res;
volatile uint8_t card_res_flag = CARD_FLAG_NONE;
volatile uint32_t rfid_last_card_tick = 0;

static unsigned char CheckSum(unsigned char *dat, unsigned char num);
void UartSendCommand(uint8_t *buff, uint8_t cnt);
uint8_t UartReceiveCommand(uint8_t data);

/* 设置读卡模块波特率为 115200（命令 0x2C） */
void SetBound115200(void)
{
	unsigned char len = 0x0A;

	Cmd.SendBuffer[0] = len;
	Cmd.SendBuffer[1] = 0x00;
	Cmd.SendBuffer[2] = 0x2C;
	Cmd.SendBuffer[3] = 0x00;
	Cmd.SendBuffer[4] = 0x01;
	Cmd.SendBuffer[5] = 0xC2;
	Cmd.SendBuffer[6] = 0x00;
	Cmd.SendBuffer[7] = 0x98;
	Cmd.SendBuffer[8] = 0x24;
	Cmd.SendBuffer[9] = 0x31;
	Cmd.SendBuffer[10] = CheckSum(Cmd.SendBuffer, len);
	UartSendCommand(Cmd.SendBuffer, len);
}

/* 设置读卡模块波特率为 9600（命令 0x2C） */
void SetBound9600(void)
{
	unsigned char len = 0x0A;

	Cmd.SendBuffer[0] = len;
	Cmd.SendBuffer[1] = 0x00;
	Cmd.SendBuffer[2] = 0x2C;
	Cmd.SendBuffer[3] = 0x00;
	Cmd.SendBuffer[4] = 0x00;
	Cmd.SendBuffer[5] = 0x25;
	Cmd.SendBuffer[6] = 0x80;
	Cmd.SendBuffer[7] = 0x98;
	Cmd.SendBuffer[8] = 0x24;
	Cmd.SendBuffer[9] = 0x31;
	Cmd.SendBuffer[10] = CheckSum(Cmd.SendBuffer, len);
	UartSendCommand(Cmd.SendBuffer, len);
}

/* 读卡号（命令 0x10） */
void ReadCard(void)
{
	unsigned char len = 3;

	Cmd.SendBuffer[0] = len;
	Cmd.SendBuffer[1] = 0;
	Cmd.SendBuffer[2] = 0x10;
	Cmd.SendBuffer[3] = CheckSum(Cmd.SendBuffer, len);
	UartSendCommand(Cmd.SendBuffer, len);
}

/* 读块数据（命令 0x11），block: 块地址 */
void ReadBlock(unsigned char block)
{
	unsigned char len = 4;

	Cmd.SendBuffer[0] = len;
	Cmd.SendBuffer[1] = 0;
	Cmd.SendBuffer[2] = 0x11;
	Cmd.SendBuffer[3] = block;
	Cmd.SendBuffer[4] = CheckSum(Cmd.SendBuffer, len);
	UartSendCommand(Cmd.SendBuffer, len);
}

/* 写块数据（命令 0x12） */
void WriteBlock(unsigned char block, unsigned char *blockData)
{
	unsigned char i;
	unsigned char len = 20;

	Cmd.SendBuffer[0] = len;
	Cmd.SendBuffer[1] = 0;
	Cmd.SendBuffer[2] = 0x12;
	Cmd.SendBuffer[3] = block;

	for(i = 0; i < 16; i ++)
	{
		Cmd.SendBuffer[4 + i] = blockData[i];
	}
	Cmd.SendBuffer[20] = CheckSum(Cmd.SendBuffer, len);
	UartSendCommand(Cmd.SendBuffer, len);
}

/* 办卡（命令 0x13） */
void MakeCard(unsigned char block, unsigned long value)
{
	unsigned char len = 9;

	Cmd.SendBuffer[0] = len;
	Cmd.SendBuffer[1] = 0;
	Cmd.SendBuffer[2] = 0x13;
	Cmd.SendBuffer[3] = block;
	Cmd.SendBuffer[4] = 0;
	Cmd.SendBuffer[5] = (unsigned char)(value >> 24);
	Cmd.SendBuffer[6] = (unsigned char)(value >> 16);
	Cmd.SendBuffer[7] = (unsigned char)(value >> 8);
	Cmd.SendBuffer[8] = (unsigned char)(value);

	Cmd.SendBuffer[9] = CheckSum(Cmd.SendBuffer, len);
	UartSendCommand(Cmd.SendBuffer, len);
}

/* 充值（命令 0x15） */
void Inc(unsigned char block, unsigned long value)
{
	unsigned char len = 8;

	Cmd.SendBuffer[0] = len;
	Cmd.SendBuffer[1] = 0;
	Cmd.SendBuffer[2] = 0x15;
	Cmd.SendBuffer[3] = block;
	Cmd.SendBuffer[4] = (unsigned char)(value >> 24);
	Cmd.SendBuffer[5] = (unsigned char)(value >> 16);
	Cmd.SendBuffer[6] = (unsigned char)(value >> 8);
	Cmd.SendBuffer[7] = (unsigned char)(value);

	Cmd.SendBuffer[8] = CheckSum(Cmd.SendBuffer, len);
	UartSendCommand(Cmd.SendBuffer, len);
}

/* 扣款（命令 0x16） */
void Dec(unsigned char block, unsigned long value)
{
	unsigned char len = 8;

	Cmd.SendBuffer[0] = len;
	Cmd.SendBuffer[1] = 0;
	Cmd.SendBuffer[2] = 0x16;
	Cmd.SendBuffer[3] = block;
	Cmd.SendBuffer[4] = (unsigned char)(value >> 24);
	Cmd.SendBuffer[5] = (unsigned char)(value >> 16);
	Cmd.SendBuffer[6] = (unsigned char)(value >> 8);
	Cmd.SendBuffer[7] = (unsigned char)(value);

	Cmd.SendBuffer[8] = CheckSum(Cmd.SendBuffer, len);
	UartSendCommand(Cmd.SendBuffer, len);
}

/* 清除卡（命令 0x14） */
void ClearCard(unsigned char block)
{
	unsigned char len = 7;

	Cmd.SendBuffer[0] = len;
	Cmd.SendBuffer[1] = 0;
	Cmd.SendBuffer[2] = 0x14;
	Cmd.SendBuffer[3] = block;
	Cmd.SendBuffer[4] = 0x38;
	Cmd.SendBuffer[5] = 0x52;
	Cmd.SendBuffer[6] = 0x7A;

	Cmd.SendBuffer[7] = CheckSum(Cmd.SendBuffer, len);
	UartSendCommand(Cmd.SendBuffer, len);
}

/* 校验：长度^地址^命令码^参数 的异或 */
static unsigned char CheckSum(unsigned char *dat, unsigned char num)
{
  unsigned char bTemp = 0, i;

  for(i = 0; i < num; i ++){bTemp ^= dat[i];}
  return bTemp;
}

/* 发送命令帧：加 0x7F 帧头，参数中的 0x7F 双写转义 */
void UartSendCommand(uint8_t *buff, uint8_t cnt)
{
  	uint8_t i;
	uint8_t data[24];
	uint8_t num = 0;

	data[0] = 0x7F;
	for( i = 0; num < cnt+1; i++ )
	{
		data[i+1] = buff[i];
		num++;
		if( buff[i] == 0x7F )
		{
			i += 1;
			data[i+1] = 0x7F;
		}
	}
	num = i+1;

	HAL_UART_Transmit( &CARD_HAL_USARTx, (uint8_t * )&data, num, 0xffff );
}

/* 逐字节解析模块响应帧（去掉 0x7F 帧头与转义），返回解析结果 */
uint8_t UartReceiveCommand(uint8_t data)
{
	static uint8_t start_receive = 0;
	static uint8_t len = 0;
	static unsigned char i = 0;

	if( start_receive == 0 )
	{
		if( data == 0x7F )	/* 帧头 */
		{
			start_receive = 1;
			len = 0;
			i = 0;
		}
		return 0;
	}
	else
	{
		if( len == 0 )
		{
			if( data< 0x7F)
			{
				len = data;
			}
		}
		else if( len > 1 )
		{
			len--;
			Cmd.ReceiveBuffer[i++] = data;
		}
		else
		{
			len = 0;
			start_receive = 0;
			if( Cmd.ReceiveBuffer[1] == 0x90 )	/* 读卡号响应 */
			{
				if( Cmd.ReceiveBuffer[2] == 0x00 )
				{
					return 2;	/* 读到卡号 */
				}
				else
				{
					return 3; /* 无卡/错误 */
				}
			}
			if( Cmd.ReceiveBuffer[1] == 0x91 )	/* 读块响应 */
			{
				if( Cmd.ReceiveBuffer[2] == 0x00 )
				{
					for( i = 0; i < 16; i++ )
					{
						Cmd.block_data[i] = Cmd.ReceiveBuffer[9+i];
					}
				}
				else
				{
					return 3; /* 无卡/错误 */
				}
			}
			else if( Cmd.ReceiveBuffer[1] == 0xAC )	/* 设波特率响应 */
			{
				if( Cmd.ReceiveBuffer[2] != 0x00 )
				{
					return 3;
				}
			}
			else
			{
				return 0;
			}

			return 1; /* 正确数据 */
		}
		return 0;
	}
}

/**
 * 串口接收完成回调（USART1 中断）。
 * 案例改造点：LEDLIGHT 态收到"读到卡号"(temp_res==2) → 刷新 rfid_last_card_tick，
 * 卡在线圈上时 LED 持续亮；"无卡"(temp_res==3) 不再主动递减熄灯，
 * 熄灭由 rfid 状态机按 HAL_GetTick() 差判断。
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if( huart->Instance == CARD_USARTx )
	{
		uint8_t temp_res = UartReceiveCommand( card_res );

		if( temp_res == 1 )	/* 收到完整读块数据 */
		{
			card_res_flag = CARD_FLAG_RESDATA;
		}
		else if( temp_res == 2 )	/* 读到卡号（卡在场） */
		{
			if( card_res_flag != CARD_FLAG_LEDLIGHT )
			{
				card_res_flag = CARD_FLAG_EXIST;
			}
			else
			{
				/* LEDLIGHT 态：刷新卡在场时间戳，保持 LED 常亮 */
				rfid_last_card_tick = HAL_GetTick();
			}
		}
		else if( temp_res == 3 )	/* 无卡/错误 */
		{
			if( card_res_flag != CARD_FLAG_LEDLIGHT )
			{
				card_res_flag = CARD_FLAG_NONE;
			}
			/* LEDLIGHT 态不再主动熄灯：由 rfid 状态机按 tick 判断 */
		}

		HAL_UART_Receive_IT(&huart1, (uint8_t *)&card_res, 1);
	}
}
