#include "motor_process.h"
#include "config.h"
#include "motor_drv.h"
#include "adc.h"
#include "debug.h"
#include "led.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"

motor_logic_t motor_control;
volatile uint8_t motor_trigger_flag = 0;

/* 时间基准：esp_timer（微秒）→ 毫秒（截断为 32 位，差值比较在回绕下仍正确） */
static inline uint32_t now_ms(void)
{
	return (uint32_t)(esp_timer_get_time() / 1000);
}

static void Motor_ApplySpeed(uint16_t speed);

/**
 * 电机初始化：电位器采样 20 次平均 → stop_time（阻塞，初始化阶段）
 */
void Motor_Init(void)
{
	uint32_t adc_value = 0;
	uint8_t  i;

	for( i = 0; i < 20; i++ )
	{
		adc_value += Get_ADC_Value();
		vTaskDelay(pdMS_TO_TICKS(1));
	}
	adc_value /= 20;

	MotorLogic_Init(&motor_control, now_ms(), MOTOR_TARGET_SPEED,
					MotorLogic_CalcStopTime(adc_value));
	Motor_ApplySpeed(0);

#if DBG_ECHO_MOTOR
	Dbg_Printf("[MOTOR] IDLE speed=0\r\n");
#endif
}

/**
 * 电机状态机（非阻塞，主循环每圈调用）：喂 motor_logic 并输出速度。
 * 时序（绝对计时）见 motor_logic.c 头部说明。
 */
void Motor_Process(void)
{
	motor_state_t old_state = motor_control.state;
	uint8_t trig = motor_trigger_flag;
	uint16_t spd;

	motor_trigger_flag = 0;             /* 触发已移交逻辑层 pending，此处清零 */
	spd = MotorLogic_Step(&motor_control, trig, now_ms());
	Motor_ApplySpeed(spd);

	if( motor_control.state != old_state )   /* 状态变化：边沿调试输出 + RGB LED 颜色 */
	{
#if DBG_ECHO_MOTOR
		Dbg_Printf("[MOTOR] %s speed=%u\r\n",
				   MotorLogic_StateName(motor_control.state), motor_control.speed);
#endif
		/* WS2812 RGB LED 颜色映射 */
		switch( motor_control.state )
		{
			case MOTOR_STATE_IDLE:     LED_SetColor(LED_COLOR_BOOT);    break;
			case MOTOR_STATE_RAMPUP:   LED_SetColor(LED_COLOR_RAMPUP);  break;
			case MOTOR_STATE_RUN:      LED_SetColor(LED_COLOR_IDLE);    break;
			case MOTOR_STATE_SLOW:     LED_SetColor(LED_COLOR_SLOWING); break;
			case MOTOR_STATE_STOPPING: LED_SetColor(LED_COLOR_SLOWING); break;
			case MOTOR_STATE_WAIT:     LED_SetColor(LED_COLOR_STOPPED); break;
			case MOTOR_STATE_STOP:     LED_SetColor(LED_COLOR_OFF);     break;
			default:                   break;
		}
	}
}

static void Motor_ApplySpeed(uint16_t speed)
{
	if( speed > MOTOR_SPEED_MAX ) speed = MOTOR_SPEED_MAX;
	Motor_Control(speed);
}

uint8_t Motor_IsInStopSequence(void)
{
	return MotorLogic_IsInStopSequence(&motor_control);
}

/* LED 占用：停车序列 + RAMPUP（缓启动期间 rfid 不得覆盖 RGB 颜色） */
uint8_t Motor_IsBusyForLed(void)
{
	return (uint8_t)(MotorLogic_IsInStopSequence(&motor_control) ||
					 motor_control.state == MOTOR_STATE_RAMPUP);
}
