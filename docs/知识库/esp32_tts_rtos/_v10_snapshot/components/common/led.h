#ifndef __LED_H
#define __LED_H

#include <stdint.h>

/* 三引脚 LED 初始化（推挽输出 + 初始高电平灭灯） */
void LED_Init(void);

/* 1=亮（低电平），0=灭 */
void LED_Sta(uint8_t sta);

#endif
