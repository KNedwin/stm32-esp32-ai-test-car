#ifndef __BSP_USART_H
#define __BSP_USART_H

#include "stm32f1xx_hal.h"

/********** 串口号设置（TTS 语音模块） ************/
#define HAL_USARTX            huart2
#define USARTX                USART2

extern UART_HandleTypeDef huart2;

void Usartx_SendString( uint8_t *str );

#endif
