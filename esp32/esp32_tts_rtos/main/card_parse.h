#ifndef __CARD_PARSE_H
#define __CARD_PARSE_H

#include <stdint.h>

/* 读卡流程标志（与 STM32 版一致） */
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
extern volatile uint8_t card_res_flag;

/* 卡在场时间戳（ms，由解析处理刷新；rfid 状态机据此判断 LED 熄灭） */
extern volatile uint32_t rfid_last_card_tick;

/* 逐字节喂入模块响应帧（含长度上限防护、仅 0x91 产生事件），
 * 内部按解析结果更新 card_res_flag / rfid_last_card_tick。
 * now_ms 为当前毫秒时间（调用方提供，便于主机测试注入）。
 * 纯逻辑，无硬件依赖，可主机单元测试。 */
void Card_Parse_Feed(uint8_t data, uint32_t now_ms);

/* 底层逐字节解析（返回 0/1/2/3，含义同 STM32 版），供测试直接使用 */
uint8_t UartReceiveCommand(uint8_t data);

#endif
