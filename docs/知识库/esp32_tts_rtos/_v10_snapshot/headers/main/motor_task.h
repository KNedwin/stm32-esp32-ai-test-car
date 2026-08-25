#ifndef __MOTOR_TASK_H
#define __MOTOR_TASK_H

#include <stdint.h>
#include "motor_logic.h"

/* 电机状态（类型与状态机逻辑在 motor_logic.h 共享） */
extern motor_logic_t motor_control;

/* 触发停车标志：由 rfid 置位，电机任务消费后清零 */
extern volatile uint8_t motor_trigger_flag;

/* 电机任务（FreeRTOS，绝对计时状态机） */
void Motor_Task(void *arg);

/* 查询是否处于停车序列（STOPPING/WAIT），供 rfid 判断是否计数 */
uint8_t Motor_IsInStopSequence(void);

/* LED 占用查询：停车序列 + RAMPUP 缓启动期间，rfid 不覆盖 RGB 颜色 */
uint8_t Motor_IsBusyForLed(void);

#endif
