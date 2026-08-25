#ifndef __MOTOR_DRV_H
#define __MOTOR_DRV_H

#include <stdint.h>

/* 电机驱动（LEDC 双通道互补，方案 A）：
 * 接口与 STM32 版 Motor_Control 完全一致（speed 0~999）
 * speed=0 → 两路同电位（高）→ 电机停；speed>0 → 一路低 + 另一路脉宽 → 差分电压
 * dir: 0=正转(CH0低+CH1脉宽)  1=反转(CH1低+CH0脉宽) */
void Motor_Drv_Init(void);
void Motor_Control(uint16_t speed);
void Motor_SetDirection(uint8_t dir);   /* 0=正转(CH0低+CH1脉宽) 1=反转(交换) */
void Motor_SetDirection(uint8_t dir);

#endif
