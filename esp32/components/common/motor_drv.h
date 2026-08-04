#ifndef __MOTOR_DRV_H
#define __MOTOR_DRV_H

#include <stdint.h>

/* 电机驱动（LEDC 双通道互补，方案 A）：
 * 接口与 STM32 版 Motor_Control 完全一致（speed 0~999）
 * speed=0 → 两路同电位（高）→ 电机停；speed>0 → CH1 低 + CH2 脉宽 → 差分电压 */
void Motor_Drv_Init(void);
void Motor_Control(uint16_t speed);

#endif
