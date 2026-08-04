#ifndef __RFID_TASK_H
#define __RFID_TASK_H

#include <stdint.h>
#include "rfid_logic.h"

typedef struct
{
	uint8_t  chinese_data[RFID_BLOCK_SIZE];  /* 本次播报缓冲（块4 或块1 的单块数据） */
	int32_t  chinese_block_num;  /* 0=读块4，-3=回退读块1 */
	int32_t  read_block;         /* 块地址基数 */

	uint8_t  wait_time;          /* 读块响应超时计数(ms) */
	uint8_t  wait_resend_times;  /* 读块响应超时重发计数 */

	uint32_t led_tick;           /* 进入 LEDLIGHT 的时刻 */

	rfid_logic_t logic;          /* 触发词/去重逻辑状态（纯逻辑层） */
} rfid_control_t;

extern rfid_control_t rfid_control;

/* 读卡任务（FreeRTOS） */
void RFID_Task(void *arg);

#endif
