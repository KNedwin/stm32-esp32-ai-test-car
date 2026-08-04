#include "BSP_ADC.h"

extern ADC_HandleTypeDef   HAL_ADCX;

/* ADC 初始化（校准） */
void ADC_Init( void )
{
  HAL_ADC_Start( &HAL_ADCX );
  HAL_ADCEx_Calibration_Start( &HAL_ADCX );
}

/* 读取一次 ADC 转换值（软件触发） */
uint32_t Get_ADC_Value( void )
{
  uint32_t adc_value = 0;

  HAL_ADC_Start( &HAL_ADCX );
  if( HAL_ADC_PollForConversion( &HAL_ADCX, 0xff ) == HAL_OK )
  {
    adc_value = HAL_ADC_GetValue( &HAL_ADCX );
  }
  return( adc_value );
}
