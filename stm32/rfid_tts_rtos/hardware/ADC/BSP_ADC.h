#ifndef __BSP_ADC_H
#define __BSP_ADC_H

#include "stm32f1xx_hal.h"

#define HAL_ADCX              hadc1

void ADC_Init( void );
uint32_t Get_ADC_Value( void );

#endif
