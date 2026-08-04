#ifndef __MOTOR_CONTROL_TASK_H
#define __MOTOR_CONTROL_TASK_H

#include "stm32f1xx_hal.h"

typedef enum
{
    MOTOR_STATE_IDLE = 0,    /* 等待晚启动 */
    MOTOR_STATE_RAMPUP,      /* 缓启动中 */
    MOTOR_STATE_RUN,         /* 正常运行 */
    MOTOR_STATE_SLOW,        /* 降速窗口内（E~E+G，开机仅一次） */
    MOTOR_STATE_STOPPING,    /* 触发停车：H 秒线性减速至 0 */
    MOTOR_STATE_WAIT,        /* 停住后静止等待 I 秒 */
    MOTOR_STATE_STOP         /* 已停止（电位器到时/1000s 上限） */
} motor_state_t;

typedef struct
{
    motor_state_t state;
    uint32_t start_tick;     /* 上电时刻（绝对计时基准，永不清零） */
    uint32_t state_tick;     /* 进入当前状态的时刻（阶段计时基准） */
    uint32_t stop_time;      /* 电位器算出的停车时间（绝对 ms） */
    uint32_t res;            /* 电位器换算阻值 */
    uint16_t target_speed;   /* 目标速度 */
    uint16_t speed;          /* 当前输出速度 */
    uint16_t ramp_start;     /* 本次缓启动的起始速度（0） */
} motor_control_t;

extern motor_control_t motor_control;

/* 触发停车标志：由 rfid 状态机置位，电机任务消费后清零 */
extern volatile uint8_t motor_trigger_flag;

/* 查询是否处于停车序列（STOPPING/WAIT），供 rfid 判断是否计数 */
uint8_t Motor_IsInStopSequence(void);

#endif
