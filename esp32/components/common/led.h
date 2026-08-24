#ifndef __LED_H
#define __LED_H

#include <stdint.h>
#include "ws2812.h"

/* 三引脚 LED 初始化（推挽输出 + 初始高电平灭灯）
 * 同时初始化板载 WS2812 RGB LED */
void LED_Init(void);

/* 1=亮（低电平），0=灭（同时驱动 WS2812 对应颜色） */
void LED_Sta(uint8_t sta);

/* 直接设置 WS2812 RGB 颜色（电机状态变化时调用） */
void LED_SetColor(led_color_t color);

#endif
