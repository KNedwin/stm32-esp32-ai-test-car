#ifndef __MOTOR_CONTROL_TASK_H
#define __MOTOR_CONTROL_TASK_H

#include "stm32f1xx_hal.h"
#include "motor_logic.h"

/* 电机状态（类型与状态机逻辑在 motor_logic.h 共享） */
extern motor_logic_t motor_control;

/* 触发停车标志：由 rfid 置位，电机任务消费后清零 */
extern volatile uint8_t motor_trigger_flag;

/* 电机初始化（电位器采样 + 状态清零），任务开头调用 */
void Motor_Init(void);

/* 电机任务（FreeRTOS，绝对计时状态机） */
void Motor_Control_Task(void const * argument);

/* 查询是否处于停车序列（STOPPING/WAIT），供 rfid 判断是否计数 */
uint8_t Motor_IsInStopSequence(void);

#endif
