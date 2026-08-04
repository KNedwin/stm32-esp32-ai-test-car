#include "debug.h"
#include "config.h"
#include <stdio.h>
#include <stdarg.h>

void Dbg_Init(void)
{
#if DBG_USART_ENABLE
	printf("[SYS] boot   LED=OFF MOTOR=STOP\r\n");
#endif
}

void Dbg_Printf(const char *fmt, ...)
{
#if DBG_USART_ENABLE
	char buf[128];
	va_list args;

	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);
	printf("%s", buf);
#endif
}
