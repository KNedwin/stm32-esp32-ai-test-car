#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led.h"
#include "motor_drv.h"
#include "debug.h"
#include "tts.h"
#include "card_uart.h"
#include "adc.h"
#include "rfid_process.h"
#include "motor_process.h"

/**
 * 裸机版入口：单任务超级循环（顺序执行两个非阻塞状态机）
 * 所有延时均非阻塞（esp_timer 时间差）；vTaskDelay(1) 让出 CPU 防看门狗。
 * 注意：FreeRTOS 默认 100Hz 节拍，vTaskDelay(1)=10ms。
 */
void app_main(void)
{
	LED_Init();                 /* LED 初始化（熄灭） */
	Motor_Drv_Init();           /* LEDC 双通道 PWM */
	Motor_Control(0);           /* 电机初始速度 0 */
	Dbg_Init();                 /* 数据输出口（[SYS] boot） */
	Card_Uart_Init();           /* 读卡 UART1 + 波特率切换 9600→115200 */
	TTS_Init();                 /* 语音 UART2 */
	ADC_Init();                 /* 电位器 ADC */

	Motor_Init();               /* 电位器采样 + 电机状态初始化（阻塞约200ms） */
	RFID_Init();                /* TTS 设置 + 读卡参数初始化（阻塞约0.9s） */

	while( 1 )
	{
		RFID_Process();         /* 读卡、播报、LED、去重、触发（非阻塞） */
		Motor_Process();        /* 电机时序（非阻塞） */
		vTaskDelay(pdMS_TO_TICKS(1));
	}
}
