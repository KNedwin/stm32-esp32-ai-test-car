#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led.h"
#include "motor_drv.h"
#include "debug.h"
#include "tts.h"
#include "card_uart.h"
#include "config_mode.h"
#include "nvs_params.h"
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

	/* 运行时参数加载（NVS 初始化 + 读参数）：必须先于 config_mode 检测 */
	params_init();
	Motor_SetDirection(g_params.motor_dir);   /* 应用电机转向参数 */

	/* 配置模式检测：连按3次RST（快速通断电）触发 */
	config_mode_boot_check();
	if( config_mode_should_enter() )
	{
		config_mode_run();      /* 阶段四替换为 WiFi+网页，当前蓝闪占位 */
	}

	/* 逻辑层时序/规则注入 */
	{
		static motor_timing_t tm;   /* static：SetTiming 保存指针，须持久 */
		uint8_t i;
		tm.late_ms = g_params.late_ms;
		tm.slow_ms = g_params.slow_ms;
		tm.stop_ramp_ms = g_params.stop_ramp_ms;
		tm.wait_ms = g_params.wait_ms;
		tm.slowwin_count = g_params.slowwin_count;
		for( i = 0; i < tm.slowwin_count && i < MOTOR_SLOWWIN_MAX; i++ )
		{
			tm.slowwin[i].start_ms = g_params.slowwins[i].start_ms;
			tm.slowwin[i].dur_ms   = g_params.slowwins[i].dur_ms;
			tm.slowwin[i].pct      = g_params.slowwins[i].pct;
		}
		MotorLogic_SetTiming(&tm);
		RfidLogic_SetConfig((const rfid_rule_rt_t *)g_params.rules,
							g_params.rule_count,
							g_params.dedup_ms, g_params.count_interval_ms);
	}

	xTaskCreatePinnedToCore(RFID_Task, "rfid", 4096, NULL, 5, NULL, tskNO_AFFINITY);
	xTaskCreatePinnedToCore(Motor_Task, "motor", 2048, NULL, 1, NULL, tskNO_AFFINITY);
}
