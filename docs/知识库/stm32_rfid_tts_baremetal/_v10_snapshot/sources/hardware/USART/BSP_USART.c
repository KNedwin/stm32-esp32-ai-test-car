#include "BSP_USART.h"
#include <stdio.h>

extern UART_HandleTypeDef HAL_USARTX;

/**
 * printf 重定向到 USART2（TTS 语音模块）。
 * CubeMX 6.17 生成的 syscalls.c 以 __io_putchar 弱符号转发 _write，
 * 此处提供强定义即可（GCC 工具链）。
 * 注意：调试输出请用 Debug.c 的 Dbg_Printf（→USART3），勿用 printf。
 */
int __io_putchar(int ch)
{
  HAL_UART_Transmit( &HAL_USARTX, (uint8_t *)&ch, 1, 0xffff );
  return ch;
}

int __io_getchar(void)
{
  uint8_t ch = 0;
  HAL_UART_Receive( &HAL_USARTX, &ch, 1, 0xffff );
  return ch;
}

/**
 * 串口数据发送函数（0 结尾字符串，逐字节发送）
 */
void Usartx_SendString( uint8_t *str )
{
  uint16_t i = 0;
  while( str[i] )
  {
    HAL_UART_Transmit( &HAL_USARTX, (uint8_t *)(str+i), 1, 0xffff );
    i++;
  }
}
