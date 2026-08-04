#include "led.h"
#include "pins.h"
#include "driver/gpio.h"
#include "debug.h"
#include "config.h"

static uint8_t led_last_sta = 0xFF;

/* 三引脚 LED（低电平亮，边沿触发调试输出） */
static void LED_Pins(uint8_t level)
{
	gpio_set_level(PIN_LED1, level);
	gpio_set_level(PIN_LED2, level);
	gpio_set_level(PIN_LED3, level);
}

void LED_Sta(uint8_t sta)
{
	switch( sta )
	{
		case 1:     /* 亮（低电平） */
			LED_Pins(0);
			break;
		case 0:     /* 灭（高电平） */
			LED_Pins(1);
			break;
		default:
			return;
	}

#if DBG_ECHO_LED
	if( led_last_sta != sta )
	{
		if( sta == 1 ) Dbg_Printf("[LED] ON\r\n");
		else           Dbg_Printf("[LED] OFF\r\n");
		led_last_sta = sta;
	}
#endif
}

/* 初始化：推挽输出 + 初始高电平（灭） */
void LED_Init(void)
{
	gpio_config_t io = {
		.pin_bit_mask = (1ULL << PIN_LED1) | (1ULL << PIN_LED2) | (1ULL << PIN_LED3),
		.mode = GPIO_MODE_OUTPUT,
		.pull_up_en = GPIO_PULLUP_ENABLE,
		.pull_down_en = GPIO_PULLDOWN_DISABLE,
		.intr_type = GPIO_INTR_DISABLE,
	};
	gpio_config(&io);
	LED_Sta(0);
}
