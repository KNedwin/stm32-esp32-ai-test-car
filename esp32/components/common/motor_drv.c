#include "motor_drv.h"
#include "pins.h"
#include "config.h"
#include "driver/ledc.h"
#include "esp_check.h"

#define DUTY_MAX    ((1U << MOTOR_PWM_RES_BITS) - 1)   /* 10bit → 1023 */

static uint8_t s_motor_dir = 0;   /* 0=正转(CH0低+CH1脉宽) 1=反转(交换) */

void Motor_SetDirection(uint8_t dir)
{
	s_motor_dir = dir & 1;
}

void Motor_Drv_Init(void)
{
	ledc_timer_config_t timer_conf = {
		.speed_mode      = MOTOR_LEDC_MODE,
		.timer_num       = MOTOR_LEDC_TIMER,
		.duty_resolution = (ledc_timer_bit_t)MOTOR_PWM_RES_BITS,
		.freq_hz         = MOTOR_PWM_FREQ_HZ,
		.clk_cfg         = LEDC_AUTO_CLK,
	};
	ESP_ERROR_CHECK(ledc_timer_config(&timer_conf));

	ledc_channel_config_t ch0 = {
		.gpio_num   = PIN_MOTOR_PWM1,
		.speed_mode = MOTOR_LEDC_MODE,
		.channel    = MOTOR_LEDC_CH0,
		.intr_type  = LEDC_INTR_DISABLE,
		.timer_sel  = MOTOR_LEDC_TIMER,
		.duty       = DUTY_MAX,
		.hpoint     = 0,
	};
	ESP_ERROR_CHECK(ledc_channel_config(&ch0));

	ledc_channel_config_t ch1 = {
		.gpio_num   = PIN_MOTOR_PWM2,
		.speed_mode = MOTOR_LEDC_MODE,
		.channel    = MOTOR_LEDC_CH1,
		.intr_type  = LEDC_INTR_DISABLE,
		.timer_sel  = MOTOR_LEDC_TIMER,
		.duty       = DUTY_MAX,
		.hpoint     = 0,
	};
	ESP_ERROR_CHECK(ledc_channel_config(&ch1));
}

void Motor_Control(uint16_t speed)
{
	uint32_t duty;

	if( speed > MOTOR_SPEED_MAX ) speed = MOTOR_SPEED_MAX;

	if( speed == 0 )
	{
		/* 两路同电位（占空比 100%）→ 电机两端电压差 0 → 停 */
		ledc_set_duty(MOTOR_LEDC_MODE, MOTOR_LEDC_CH0, DUTY_MAX);
		ledc_set_duty(MOTOR_LEDC_MODE, MOTOR_LEDC_CH1, DUTY_MAX);
	}
	else if( s_motor_dir == 0 )
	{
		/* 正转：CH0 恒低，CH1 脉宽 → 差分电压 = speed/999 × Vcc */
		ledc_set_duty(MOTOR_LEDC_MODE, MOTOR_LEDC_CH0, 0);
		duty = (uint32_t)speed * DUTY_MAX / MOTOR_SPEED_MAX;
		ledc_set_duty(MOTOR_LEDC_MODE, MOTOR_LEDC_CH1, duty);
	}
	else
	{
		/* 反转：CH1 恒低，CH0 脉宽 */
		ledc_set_duty(MOTOR_LEDC_MODE, MOTOR_LEDC_CH1, 0);
		duty = (uint32_t)speed * DUTY_MAX / MOTOR_SPEED_MAX;
		ledc_set_duty(MOTOR_LEDC_MODE, MOTOR_LEDC_CH0, duty);
	}
	ledc_update_duty(MOTOR_LEDC_MODE, MOTOR_LEDC_CH0);
	ledc_update_duty(MOTOR_LEDC_MODE, MOTOR_LEDC_CH1);
}
