#ifndef __DEBUG_H
#define __DEBUG_H

#include <stdint.h>

/* 数据输出口：USB-Serial-JTAG console（printf）。格式与 STM32 版一致：
 * [SYS] boot / [RFID] xx xx / [LED] ON|OFF / [MOTOR] STATE speed=n */
void Dbg_Init(void);
void Dbg_Printf(const char *fmt, ...);

#endif
