#ifndef __PWM_H
#define __PWM_H

#include "stm32f1xx_hal.h"

#define PWM_HAL_TIMX          htim2
#define PWM_TIMX              TIM2
#define PWM_CHANNELX          TIM_CHANNEL_1
#define PWM_CHANNELX2         TIM_CHANNEL_2

void PWM_Init( void );
void PWM_DutySet( TIM_TypeDef* timx, uint8_t channel, uint16_t duty );
void Motor_Control( uint16_t speed );
void Motor_SetDirection( uint8_t dir );   /* 0=正转(CH1低+CH2脉宽) 1=反转(交换) */

#endif
