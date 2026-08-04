/* U13T 读卡模块帧解析（纯逻辑，无硬件依赖，可主机单元测试）
 * 移植自 STM32 版 Card.c，含：长度上限防护、仅 0x91 产生事件、LEDLIGHT 卡在场刷新
 */
#include "card_parse.h"

CMD Cmd;
volatile uint8_t card_res_flag = CARD_FLAG_NONE;
volatile uint32_t rfid_last_card_tick = 0;

/* 逐字节解析模块响应帧（去掉 0x7F 帧头与转义），返回解析结果（与 STM32 版一致） */
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
			if( data < 0x7F )
			{
				/* 长度上限校验：ReceiveBuffer[32] 放不下即丢弃整帧 */
				if( data > 31 )
				{
					start_receive = 0;
					len = 0;
					i = 0;
					return 0;
				}
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
					return 1; /* 读到块数据 */
				}
				else
				{
					return 3; /* 无卡/错误 */
				}
			}
			/* 其他响应（0xAC 设波特率等）不产生事件 */
			return 0;
		}
		return 0;
	}
}

/* 喂字节 + 按解析结果更新状态标志（flag / 卡在场时间戳） */
void Card_Parse_Feed(uint8_t data, uint32_t now_ms)
{
	uint8_t res = UartReceiveCommand(data);

	if( res == 1 )	/* 收到完整读块数据 */
	{
		card_res_flag = CARD_FLAG_RESDATA;
	}
	else if( res == 2 )	/* 读到卡号（卡在场） */
	{
		if( card_res_flag != CARD_FLAG_LEDLIGHT )
		{
			card_res_flag = CARD_FLAG_EXIST;
		}
		else
		{
			/* LEDLIGHT 态：刷新卡在场时间戳，保持 LED 常亮 */
			rfid_last_card_tick = now_ms;
		}
	}
	else if( res == 3 )	/* 无卡/错误 */
	{
		if( card_res_flag != CARD_FLAG_LEDLIGHT )
		{
			card_res_flag = CARD_FLAG_NONE;
		}
		/* LEDLIGHT 态不主动熄灯：由 rfid 状态机按时间差判断 */
	}
}
