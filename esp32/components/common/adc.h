#ifndef __ADC_H
#define __ADC_H

#include <stdint.h>

/* 电位器采样（SARADC1_CH0，12-bit） */
void ADC_Init(void);
uint32_t Get_ADC_Value(void);

#endif
