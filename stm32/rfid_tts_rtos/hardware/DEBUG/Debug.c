#include "Debug.h"
#include "stm32f1xx_hal.h"
#include "config.h"
#include <stdio.h>
#include <stdarg.h>

extern UART_HandleTypeDef huart3;

/* 注意：dbg_buf 用函数内局部变量（可重入）。
 * RTOS 版两任务并发调用 Dbg_Printf 时，HAL_UART_Transmit 轮询模式对占用中的
 * 串口会返回 BUSY 丢弃本次输出（不交错数据），仅可能丢一条调试信息。 */

/**
 * 数据输出串口初始化（USART3，115200）。
 * 由 CubeMX 的 MX_USART3_UART_Init 完成外设配置，这里只输出上电信息。
 */
void Dbg_Init(void)
{
#if DBG_USART_ENABLE
  Dbg_Printf("[SYS] boot   LED=OFF MOTOR=STOP\r\n");
#endif
}

/**
 * 数据输出串口打印（USART3）。
 * 事件格式：见各版 docs/02-项目说明.md 第5节。
 * 总开关由 DBG_USART_ENABLE 控制；分类开关（DBG_ECHO_*）由调用方判断。
 */
void Dbg_Printf(const char *fmt, ...)
{
#if DBG_USART_ENABLE
  char dbg_buf[128];
  va_list args;
  uint16_t len;

  va_start(args, fmt);
  len = (uint16_t)vsnprintf(dbg_buf, sizeof(dbg_buf), fmt, args);
  va_end(args);

  if( len > sizeof(dbg_buf) - 1 ) len = sizeof(dbg_buf) - 1;
  if( len > 0 )
  {
    HAL_UART_Transmit(&huart3, (uint8_t *)dbg_buf, len, 100);
  }
#endif
}
