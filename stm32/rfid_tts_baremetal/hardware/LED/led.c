#include "led.h"
#include "Debug.h"
#include "config.h"

static uint8_t led_last_sta = 0xFF;

/* 控制 LED 亮灭：1=亮（低电平），0=灭（高电平）。三引脚兼容。
 * 调试输出为边沿触发（状态变化才输出一次，防轮询刷屏）。 */
void LED_Sta( uint8_t sta )
{
    switch( sta )
    {
        case 1:     /* 灯亮 */
        {
            HAL_GPIO_WritePin( GPIOC, GPIO_PIN_13, GPIO_PIN_RESET );
            HAL_GPIO_WritePin( GPIOB, GPIO_PIN_12, GPIO_PIN_RESET );
            HAL_GPIO_WritePin( GPIOA, GPIO_PIN_8, GPIO_PIN_RESET );
            break;
        }
        case 0:     /* 灯灭 */
        {
            HAL_GPIO_WritePin( GPIOC, GPIO_PIN_13, GPIO_PIN_SET );
            HAL_GPIO_WritePin( GPIOB, GPIO_PIN_12, GPIO_PIN_SET );
            HAL_GPIO_WritePin( GPIOA, GPIO_PIN_8, GPIO_PIN_SET );
            break;
        }
        default:
        {
            return;
        }
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

/* 三引脚电平翻转 */
void LED_Toggle( void )
{
    HAL_GPIO_TogglePin( GPIOC, GPIO_PIN_13 );
    HAL_GPIO_TogglePin( GPIOB, GPIO_PIN_12 );
    HAL_GPIO_TogglePin( GPIOA, GPIO_PIN_8 );
}
