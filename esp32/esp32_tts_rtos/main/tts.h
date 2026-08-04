#ifndef __TTS_H
#define __TTS_H

#include <stdint.h>

/* CN-TTS 语音模块（UART2，9600）：初始化 + 发送 */
void TTS_Init(void);

/* 发送 0 结尾 GBK 字符串并等待发送完成 */
void TTS_Send(const uint8_t *str);

#endif
