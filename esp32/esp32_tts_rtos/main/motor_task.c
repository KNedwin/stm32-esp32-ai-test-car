#include "motor_task.h"
#include "config.h"
#include "motor_drv.h"
#include "adc.h"
#include "debug.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"

motor_control_t motor_control;
volatile uint8_t motor_trigger_flag = 0;

/* 时间基准：esp_timer（微秒）→ 毫秒，64 位无回绕 */
static inline uint32_t now_ms(void)
{
	return (uint32_t)(esp_timer_get_time() / 1000);
}

static void Motor_ApplySpeed(uint16_t speed);

void Motor_Init(void)
{
	uint32_t adc_value = 0;
	int32_t  r;
	uint8_t  i;

	for( i = 0; i < 20; i++ )
	{
		adc_value += Get_ADC_Value();
		vTaskDelay(pdMS_TO_TICKS(1));
	}
	adc_value /= 20;

	/* 电位器换算阻值（clamp 保护：adc→4095 时原公式会整数溢出为负） */
	if( adc_value >= 4095UL ) r = 0;
	else
	{
		r = 5000 - (int32_t)adc_value * 1000 / (int32_t)(4095 - adc_value);
		if( r < 0 ) r = 0;
		if( r > RES_MAX ) r = RES_MAX;
	}

	motor_control.res = (uint32_t)r;
	motor_control.stop_time = STOP_TIME_MIN_MS
		+ (uint32_t)r * (STOP_TIME_MAX_MS - STOP_TIME_MIN_MS) / RES_MAX;

	motor_control.state = MOTOR_STATE_IDLE;
	motor_control.target_speed = MOTOR_TARGET_SPEED;
	motor_control.speed = 0;
	motor_control.start_tick = now_ms();
	motor_control.state_tick = motor_control.start_tick;
	Motor_ApplySpeed(0);

#if DBG_ECHO_MOTOR
	Dbg_Printf("[MOTOR] IDLE speed=0\r\n");
#endif
}

/**
 * 电机任务（绝对计时，start_tick 上电记录永不清零）：
 * IDLE →(t≥A)→ RAMPUP →(t≥A+B)→ RUN
 * RUN 内 t∈[E,E+G) → SLOW（开机仅一次）
 * motor_trigger_flag → STOPPING(H秒减速) → WAIT(I秒静止) → RAMPUP 重新缓启动
 * t≥stop_time 或 t≥1000s → STOP
 */
void Motor_Task(void *arg)
{
	uint32_t t, progress;
	uint16_t ramp_speed;

	Motor_Init();

	for(;;)
	{
		t = now_ms() - motor_control.start_tick;
		progress = now_ms() - motor_control.state_tick;

		switch( motor_control.state )
		{
			case MOTOR_STATE_IDLE:
				if( t >= MOTOR_START_LATE_TIME_MS )
				{
					motor_control.state = MOTOR_STATE_RAMPUP;
					motor_control.state_tick = now_ms();
#if DBG_ECHO_MOTOR
					Dbg_Printf("[MOTOR] RAMPUP speed=%u\r\n", motor_control.speed);
#endif
				}
				break;

			case MOTOR_STATE_RAMPUP:
				ramp_speed = (uint16_t)((uint32_t)motor_control.target_speed * progress
										/ MOTOR_START_SLOW_TIME_MS);
				if( ramp_speed >= motor_control.target_speed )
				{
					ramp_speed = motor_control.target_speed;
					motor_control.state = MOTOR_STATE_RUN;
					motor_control.state_tick = now_ms();
#if DBG_ECHO_MOTOR
					Dbg_Printf("[MOTOR] RUN speed=%u\r\n", ramp_speed);
#endif
				}
				Motor_ApplySpeed(ramp_speed);
				break;

			case MOTOR_STATE_RUN:
			case MOTOR_STATE_SLOW:
				if( motor_trigger_flag )
				{
					motor_trigger_flag = 0;
					motor_control.state = MOTOR_STATE_STOPPING;
					motor_control.state_tick = now_ms();
					motor_control.ramp_start = motor_control.speed;
#if DBG_ECHO_MOTOR
					Dbg_Printf("[MOTOR] STOPPING speed=%u\r\n", motor_control.speed);
#endif
					break;
				}
				if( (t >= (uint32_t)MOTOR_TIME_START_S*1000) &&
					(t < (uint32_t)(MOTOR_TIME_START_S+MOTOR_TIME_DURATION_S)*1000) )
				{
					if( motor_control.state != MOTOR_STATE_SLOW )
					{
						motor_control.state = MOTOR_STATE_SLOW;
#if DBG_ECHO_MOTOR
						Dbg_Printf("[MOTOR] SLOW speed=%u\r\n",
								   (uint16_t)(motor_control.target_speed*MOTOR_SPEED_PERCENT/100));
#endif
					}
					Motor_ApplySpeed((uint16_t)(motor_control.target_speed*MOTOR_SPEED_PERCENT/100));
				}
				else
				{
					if( motor_control.state != MOTOR_STATE_RUN )
					{
						motor_control.state = MOTOR_STATE_RUN;
#if DBG_ECHO_MOTOR
						Dbg_Printf("[MOTOR] RUN speed=%u\r\n", motor_control.target_speed);
#endif
					}
					Motor_ApplySpeed(motor_control.target_speed);
				}
				if( (t >= motor_control.stop_time) || (t >= MOTOR_MAX_RUN_TIME_MS) )
				{
					motor_control.state = MOTOR_STATE_STOP;
					Motor_ApplySpeed(0);
#if DBG_ECHO_MOTOR
					Dbg_Printf("[MOTOR] STOP speed=0\r\n");
#endif
				}
				break;

			case MOTOR_STATE_STOPPING:
				if( progress >= (uint32_t)TRIGGER_STOP_RAMP_TIME_S*1000UL )
				{
					motor_control.state = MOTOR_STATE_WAIT;
					motor_control.state_tick = now_ms();
					Motor_ApplySpeed(0);
#if DBG_ECHO_MOTOR
					Dbg_Printf("[MOTOR] WAIT speed=0\r\n");
#endif
				}
				else
				{
					uint32_t remain = (uint32_t)TRIGGER_STOP_RAMP_TIME_S*1000UL - progress;
					Motor_ApplySpeed((uint16_t)((uint32_t)motor_control.ramp_start
												* remain
												/ ((uint32_t)TRIGGER_STOP_RAMP_TIME_S*1000UL)));
				}
				break;

			case MOTOR_STATE_WAIT:
				if( progress >= (uint32_t)TRIGGER_WAIT_TIME_S*1000UL )
				{
					motor_control.state = MOTOR_STATE_RAMPUP;
					motor_control.state_tick = now_ms();
#if DBG_ECHO_MOTOR
					Dbg_Printf("[MOTOR] RAMPUP speed=0\r\n");
#endif
				}
				break;

			case MOTOR_STATE_STOP:
			default:
				Motor_ApplySpeed(0);
				break;
		}

		vTaskDelay(pdMS_TO_TICKS(1));
	}
}

static void Motor_ApplySpeed(uint16_t speed)
{
	if( speed > 999 ) speed = 999;
	motor_control.speed = speed;
	Motor_Control(speed);
}

uint8_t Motor_IsInStopSequence(void)
{
	return (motor_control.state == MOTOR_STATE_STOPPING) ||
		   (motor_control.state == MOTOR_STATE_WAIT);
}
