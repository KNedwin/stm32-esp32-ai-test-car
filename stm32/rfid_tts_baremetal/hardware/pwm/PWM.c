#include "PWM.h"

static uint8_t s_motor_dir = 0;   /* 0=正转 1=反转 */

void Motor_SetDirection( uint8_t dir )
{
	s_motor_dir = dir & 1;
}

extern TIM_HandleTypeDef PWM_HAL_TIMX;

/* PWM 初始化：启动双通道（案例原样） */
void PWM_Init( void )
{
  HAL_TIM_PWM_Start( &PWM_HAL_TIMX, PWM_CHANNELX );
  HAL_TIM_PWM_Start( &PWM_HAL_TIMX, PWM_CHANNELX2 );
}

/* 占空比设置（PWM2 模式下 CCR 越大高电平时间越长） */
void PWM_DutySet( TIM_TypeDef* timx, uint8_t channel, uint16_t duty )
{
  switch( channel )
  {
    case 1: timx->CCR1 = duty; break;
    case 2: timx->CCR2 = duty; break;
    case 3: timx->CCR3 = duty; break;
    case 4: timx->CCR4 = duty; break;
  }
}

/**
 * 电机控制。
 * PWM2 模式下 CCR=999 → 高电平占空比接近 100%（两端同电位）。
 *   speed==0：两路都 CCR=999 → 电机两端同电位，停转
 *   speed>0 ：CH1 CCR=0（低），CH2 CCR=speed（脉宽）
 *             → 两端电压差 = speed/999 × 3.3V，线性调速
 */
void Motor_Control( uint16_t speed )
{
	uint16_t set_speed = speed;

	if( set_speed > 999 )	set_speed = 999;

	if( speed == 0 )
	{
		PWM_DutySet( PWM_TIMX, 1, 999 );
		PWM_DutySet( PWM_TIMX, 2, 999 );
	}
	else if( s_motor_dir == 0 )
	{	/* 正转：CH1 低 + CH2 脉宽 */
		PWM_DutySet( PWM_TIMX, 1, 0 );
		PWM_DutySet( PWM_TIMX, 2, speed );
	}
	else
	{	/* 反转：CH1 脉宽 + CH2 低 */
		PWM_DutySet( PWM_TIMX, 1, speed );
		PWM_DutySet( PWM_TIMX, 2, 0 );
	}
}
