#ifndef __PINS_H
#define __PINS_H

/* ESP32-S3 硬件映射（修改后重新编译即可）
 * 集中管理：GPIO 引脚 + 外设实例（UART/ADC/LEDC 通道） */

#include "driver/gpio.h"
#include "driver/uart.h"
#include "driver/ledc.h"
#include "esp_adc/adc_oneshot.h"

/* ---------- GPIO 引脚 ---------- */
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

/* ---------- 外设实例 ---------- */
#define RFID_UART        UART_NUM_1    /* 读卡模块串口 */
#define TTS_UART         UART_NUM_2    /* 语音模块串口 */
#define ADC_UNIT         ADC_UNIT_1    /* 电位器 ADC 单元 */
#define ADC_CHANNEL      ADC_CHANNEL_0 /* 电位器 ADC 通道（=GPIO1） */

#define MOTOR_PWM_FREQ_HZ   20000      /* 电机 PWM 频率 */
#define MOTOR_PWM_RES_BITS  10         /* LEDC 分辨率位（0~1023） */
#define MOTOR_LEDC_MODE     LEDC_LOW_SPEED_MODE  /* ESP32-S3 仅低俗模式 */
#define MOTOR_LEDC_TIMER    LEDC_TIMER_0
#define MOTOR_LEDC_CH0      LEDC_CHANNEL_0
#define MOTOR_LEDC_CH1      LEDC_CHANNEL_1

#endif
