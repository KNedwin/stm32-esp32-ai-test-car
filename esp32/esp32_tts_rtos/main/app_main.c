#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led.h"
#include "motor_drv.h"
#include "debug.h"
#include "tts.h"
#include "card_uart.h"
#include "adc.h"
#include "rfid_task.h"
#include "motor_task.h"

/* LED 初始化在 led.c 内定义 */
void LED_Init(void);

/**
 * RTOS 版入口：外设初始化 + 创建两个任务
 *  - RFID_Task（优先级 5，栈 4096B）：读卡/播报/LED/触发
 *  - Motor_Task（优先级 1，栈 2048B）：电机绝对计时状态机
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

	xTaskCreatePinnedToCore(RFID_Task, "rfid", 4096, NULL, 5, NULL, tskNO_AFFINITY);
	xTaskCreatePinnedToCore(Motor_Task, "motor", 2048, NULL, 1, NULL, tskNO_AFFINITY);
}
