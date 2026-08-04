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

#endif
