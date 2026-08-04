#ifndef __MOTOR_LOGIC_H
#define __MOTOR_LOGIC_H

#include "config.h"
#include <stdint.h>

/* 电机状态枚举（四版共享：STM32/ESP32 × RTOS/裸机） */
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

/* 电机状态机纯逻辑状态（无硬件/OS 依赖，主机可测） */
typedef struct
{
    motor_state_t state;
    uint32_t start_tick;     /* 上电时刻（绝对计时基准，永不清零） */
    uint32_t state_tick;     /* 进入当前状态的时刻（阶段计时基准） */
    uint32_t stop_time;      /* 电位器算出的停车时间（绝对 ms） */
    uint32_t res;            /* 电位器换算阻值 */
    uint16_t target_speed;   /* 目标速度 */
    uint16_t speed;          /* 当前输出速度 */
    uint16_t ramp_start;     /* 减速起点速度（进入 STOPPING 时记录） */
    uint8_t  pending_trigger;/* 未消费的触发请求（IDLE/RAMPUP 期挂起，RUN/SLOW 消费） */
} motor_logic_t;

/* 初始化：记录绝对计时起点与目标速度（应用层先算好 stop_time） */
void MotorLogic_Init(motor_logic_t *lx, uint32_t now_ms, uint16_t target_speed, uint32_t stop_time);

/* 每拍调用一次：trigger 并入 pending（应用层传入后自行清零自身标志），
 * 返回本拍应输出速度（0~999）。状态机时序见 docs（绝对计时）。 */
uint16_t MotorLogic_Step(motor_logic_t *lx, uint8_t trigger, uint32_t now_ms);

/* 是否处于停车序列（STOPPING/WAIT），供 rfid 判断是否计数 */
uint8_t MotorLogic_IsInStopSequence(const motor_logic_t *lx);

/* 电位器采样均值 → stop_time（clamp 保护：adc→4095 原公式会溢出为负） */
uint32_t MotorLogic_CalcStopTime(uint32_t adc_avg);

/* 状态名（调试输出用） */
const char *MotorLogic_StateName(motor_state_t s);

#endif
