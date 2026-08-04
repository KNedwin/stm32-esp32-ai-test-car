#ifndef __RFID_PROCESS_H
#define __RFID_PROCESS_H

#include "stm32f1xx_hal.h"

typedef struct
{
    uint8_t  chinese_data[16];   /* 本次播报缓冲（块4 或块1 的单块数据） */
    int32_t  chinese_block_num;  /* 0=读块4，-3=回退读块1 */
    int32_t  read_block;         /* 块地址基数 */

    uint8_t  wait_time;          /* 兼容案例字段（未用） */
    uint8_t  wait_resend_times;  /* 读块响应超时重发计数 */

    /* LED 保持（tick 法） */
    uint32_t led_tick;           /* 进入 LEDLIGHT 的时刻 */
    uint32_t poll_tick;          /* LED 保持轮询读卡号时刻 */

    /* 播报去重 */
    uint8_t  last_speak[16];     /* 上次播报内容 */
    uint32_t last_speak_tick;    /* 上次播报时刻 */

    /* 触发词状态（数组容量按 TRIGGER_RULES 条数） */
    uint8_t  trig_count[4];              /* 已计次数 */
    uint32_t trig_last_count_tick[4];    /* 上次有效计数时刻 */
    uint8_t  trig_triggered[4];          /* 已触发标志（上电周期内保持） */
} rfid_control_t;

extern rfid_control_t rfid_control;

void RFID_Init(void);      /* TTS 设置 + 参数初始化（阻塞，初始化阶段调用） */
void RFID_Process(void);   /* 非阻塞状态机，主循环每圈调用 */

#endif
