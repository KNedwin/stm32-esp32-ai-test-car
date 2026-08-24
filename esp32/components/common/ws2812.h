#ifndef __WS2812_H
#define __WS2812_H

#include <stdint.h>

/* ============ 板载 RGB LED（WS2812 on GPIO48）状态颜色 ============ */
typedef enum {
    LED_COLOR_BOOT = 0,    /* 蓝色：开机初始化 */
    LED_COLOR_IDLE,        /* 微白：空闲运行 */
    LED_COLOR_CARD,        /* 绿色：读到卡 */
    LED_COLOR_SLOWING,     /* 黄色：触发减速 STOPPING */
    LED_COLOR_STOPPED,     /* 红色：静止 WAIT */
    LED_COLOR_RAMPUP,      /* 橙色：重新缓启动 */
    LED_COLOR_OFF,         /* 熄灭 */
    LED_COLOR_COUNT
} led_color_t;

/* 初始化 WS2812（RMT TX 驱动） */
void WS2812_Init(void);

/* 设置 RGB LED 颜色（按枚举） */
void WS2812_SetColor(led_color_t color);

#endif /* __WS2812_H */
