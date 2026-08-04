#include "PWM.h"

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
 * 电机控制（案例原样）。
 * speed 0~999：CH1 恒低（CCR=0，PWM2 下输出高电平... 详见注释）
 * 说明：PWM2 模式下 CCR=999 → 高电平占空比接近 100%。
 *   speed==0：两路都 CCR=999 → 两端同电位，电机停
 *   speed>0 ：CH1=0（低），CH2=speed（脉宽）→ 两端电压差 = speed/999 × 3.3V
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
	else
	{
		PWM_DutySet( PWM_TIMX, 1, 0 );
		PWM_DutySet( PWM_TIMX, 2, speed );
	}
}
