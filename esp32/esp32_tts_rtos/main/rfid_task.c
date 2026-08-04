#include "rfid_task.h"
#include "motor_task.h"
#include "config.h"
#include "card_uart.h"
#include "card_parse.h"
#include "tts.h"
#include "led.h"
#include "debug.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include <string.h>

rfid_control_t rfid_control;

extern volatile uint8_t card_res_flag;
extern volatile uint32_t rfid_last_card_tick;

static inline uint32_t now_ms(void)
{
	return (uint32_t)(esp_timer_get_time() / 1000);
}

static void RFID_Par_Init(void);
static void RFID_HandleCardData(void);
static void RFID_Speak(const uint8_t *data);
static void BufClear(uint8_t *buf);

/**
 * 读卡任务（与 STM32 版 rfid_task 相同逻辑）：
 * EXIST(读块) → WAIT(等响应) → RESDATA(处理) → LEDLIGHT(亮灯轮询保持) → NONE
 */
void RFID_Task(void *arg)
{
	vTaskDelay(pdMS_TO_TICKS(500));     /* 等待语音模块初始化 */

#if RFID_SETTING_SPEAK_SPEED
	/* 语速/音量/上电提示设置（断电保存） */
	TTS_Send((const uint8_t *)"<S>3");
	vTaskDelay(pdMS_TO_TICKS(80));
	TTS_Send((const uint8_t *)"<V>6");
	vTaskDelay(pdMS_TO_TICKS(80));
	TTS_Send((const uint8_t *)"<I>0");
	vTaskDelay(pdMS_TO_TICKS(200));
#endif

	RFID_Par_Init();

	for(;;)
	{
		Card_Uart_Poll();    /* 消费读卡串口接收（非阻塞） */

		if( card_res_flag == CARD_FLAG_EXIST )               /* 检测到卡，读块 */
		{
			BufClear( Cmd.block_data );
			Card_ReadBlock( rfid_control.read_block + rfid_control.chinese_block_num );
			card_res_flag = CARD_FLAG_WAIT;
			rfid_control.wait_time = 0;
			/* 注意：wait_resend_times 不在 EXIST 清零，保证"重发2次后放弃"可达 */
		}
		else if( card_res_flag == CARD_FLAG_RESDATA )        /* 收到读块数据 */
		{
			if( rfid_control.chinese_block_num == 0 && !Cmd.block_data[0] )
			{
				/* 块4无数据 → 回退读块1 */
				rfid_control.chinese_block_num = -3;
				card_res_flag = CARD_FLAG_EXIST;
			}
			else if( rfid_control.chinese_block_num == -3 && !Cmd.block_data[0] )
			{
				/* 块1也无数据 */
				card_res_flag = CARD_FLAG_NONE;
			}
			else
			{
				/* 单块数据：统一拷贝到 chinese_data 后处理 */
				memcpy(rfid_control.chinese_data, Cmd.block_data, RFID_BLOCK_SIZE);
				rfid_control.chinese_data[RFID_BLOCK_SIZE-1] = 0;  /* 强制 0 结尾，防播报越界 */
				RFID_HandleCardData();
				card_res_flag = CARD_FLAG_LEDLIGHT;
				rfid_control.led_tick = now_ms();
				rfid_last_card_tick = now_ms();   /* 重置卡在场计时起点 */
			}
		}
		else if( card_res_flag == CARD_FLAG_WAIT )           /* 等待模块响应 */
		{
			static uint32_t wait_tick = 0;
			if( (now_ms() - wait_tick) >= 1UL )              /* 1ms 节拍 */
			{
				wait_tick = now_ms();
				rfid_control.wait_time++;
				if( rfid_control.wait_time >= RFID_READ_TIMEOUT_MS )  /* 超时重发 */
				{
					rfid_control.wait_resend_times++;
					card_res_flag = CARD_FLAG_EXIST;
				}
				if( rfid_control.wait_resend_times >= 2 )    /* 重发2次后放弃 */
				{
					rfid_control.wait_resend_times = 0;
					rfid_control.wait_time = 0;
					card_res_flag = CARD_FLAG_NONE;
				}
			}
		}

		if( card_res_flag == CARD_FLAG_NONE )                /* 无卡：清状态 */
		{
			BufClear( rfid_control.chinese_data );
			rfid_control.chinese_block_num = 0;
			LED_Sta( 0 );
			rfid_control.wait_time = 0;
			rfid_control.wait_resend_times = 0;
		}
		else if( card_res_flag == CARD_FLAG_LEDLIGHT )       /* LED 保持：轮询读卡号 */
		{
			LED_Sta( 1 );
			if( (now_ms() - rfid_control.led_tick) >= (uint32_t)RFID_READ_DELAY_MS )
			{
				/* 播报延时结束，开始轮询读卡号维持 LED */
				static uint32_t poll_tick = 0;
				if( (now_ms() - poll_tick) >= RFID_LED_POLL_MS )
				{
					poll_tick = now_ms();
					Card_ReadCard();
				}
				/* 卡在场由 Card_Uart_Poll 刷新 rfid_last_card_tick；脱离 C 秒后熄灭 */
				if( (now_ms() - rfid_last_card_tick) >= (uint32_t)LED_ON_TIME_S*1000UL )
				{
					LED_Sta( 0 );
					card_res_flag = CARD_FLAG_NONE;
				}
			}
		}

		vTaskDelay(pdMS_TO_TICKS(1));
	}
}

static void RFID_Par_Init(void)
{
	rfid_control.chinese_block_num = 0;
	rfid_control.read_block = 4;
	rfid_control.wait_time = 0;
	rfid_control.wait_resend_times = 0;
	rfid_control.led_tick = 0;
	RfidLogic_Init(&rfid_control.logic);
	rfid_last_card_tick = now_ms();

#if RFID_READ_DATA_WHEN_START
	card_res_flag = CARD_FLAG_EXIST;
#else
	card_res_flag = CARD_FLAG_NONE;
#endif
}

/* 卡数据处理：决策由 rfid_logic 完成，本函数执行事件动作（置标志/播报） */
static void RFID_HandleCardData(void)
{
	uint8_t ev = RfidLogic_Process(&rfid_control.logic, rfid_control.chinese_data,
								   RFID_BLOCK_SIZE, Motor_IsInStopSequence(),
								   now_ms());

#if DBG_ECHO_RFID
	Dbg_Printf("[RFID] ");
	for( uint8_t i = 0; i < RFID_BLOCK_SIZE && rfid_control.chinese_data[i]; i++ )
	{
		Dbg_Printf("%02X ", rfid_control.chinese_data[i]);
	}
	Dbg_Printf("\r\n");
#endif

	if( ev & RFID_EV_TRIGGER_STOP )
	{
		motor_trigger_flag = 1;
	}
	if( ev & (RFID_EV_SPEAK | RFID_EV_SPEAK_FORCED) )
	{
		RFID_Speak(rfid_control.chinese_data);
	}
	/* ev == RFID_EV_NONE：去重拦截，不播报（LED 由 LEDLIGHT 逻辑照常亮） */
}

/* 直接送 TTS 播报，并更新去重记录 */
static void RFID_Speak(const uint8_t *data)
{
	TTS_Send(data);
	RfidLogic_UpdateSpeak(&rfid_control.logic, data, now_ms());
}

/* 清空缓冲（清到首个 0，最多 RFID_BLOCK_SIZE 字节） */
static void BufClear(uint8_t *buf)
{
	uint8_t k = 0;
	for( k = 0; k < RFID_BLOCK_SIZE && buf[k] != '\0'; k++ )
	{
		buf[k] = 0;
	}
}
