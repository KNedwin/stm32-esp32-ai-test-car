#ifndef __PINS_H
#define __PINS_H

/* ESP32-S3 引脚分配（修改后重新编译即可，两版项目各自维护） */

#include "driver/gpio.h"

#define PIN_RFID_TX      GPIO_NUM_10   /* U13T 读卡模块 UART1 TX */
#define PIN_RFID_RX      GPIO_NUM_11   /* U13T 读卡模块 UART1 RX */
#define PIN_TTS_TX       GPIO_NUM_12   /* CN-TTS 语音模块 UART2 TX */
#define PIN_TTS_RX       GPIO_NUM_13   /* CN-TTS 语音模块 UART2 RX */
#define PIN_MOTOR_PWM1   GPIO_NUM_4    /* 电机 LEDC 通道 0 */
#define PIN_MOTOR_PWM2   GPIO_NUM_5    /* 电机 LEDC 通道 1 */
#define PIN_ADC_POT      GPIO_NUM_1    /* 电位器 SARADC1_CH0 */
#define PIN_LED1         GPIO_NUM_2    /* LED（低电平亮） */
#define PIN_LED2         GPIO_NUM_8
#define PIN_LED3         GPIO_NUM_9

#define MOTOR_PWM_FREQ_HZ   20000      /* 电机 PWM 频率 */
#define MOTOR_PWM_RES_BITS  10         /* LEDC 分辨率位（0~1023） */

#endif
