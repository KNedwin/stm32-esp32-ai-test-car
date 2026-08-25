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

/* 绿色确认窗：触发后先亮绿 TRIGGER_ACK_GREEN_MS 再启动停车序列 */
static uint8_t  s_trig_pending = 0;
static uint32_t s_trig_pending_tick = 0;

/* 时间基准：esp_timer（微秒）→ 毫秒（截断为 32 位，差值比较在回绕下仍正确） */
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
 * 注意：FreeRTOS 默认 100Hz 节拍，vTaskDelay(1)=10ms；所有超时用真实时间差判断。
 */
void RFID_Task(void *arg)
{
	vTaskDelay(pdMS_TO_TICKS(500));     /* 等待语音模块初始化 */

	TTS_SetupDefaults();                /* 语速/音量/上电提示设置（模块指令已下沉 tts.c） */

	RFID_Par_Init();

	for(;;)
	{
		/* 绿色确认窗到期 → 正式触发停车序列 */
		if( s_trig_pending && (now_ms() - s_trig_pending_tick) >= (uint32_t)TRIGGER_ACK_GREEN_MS )
		{
			motor_trigger_flag = 1;
			s_trig_pending = 0;
		}

		Card_Uart_Poll();    /* 消费读卡串口接收（非阻塞） */

		if( card_res_flag == CARD_FLAG_EXIST )               /* 检测到卡，读块 */
		{
			BufClear( Cmd.block_data );
			Card_ReadBlock( rfid_control.read_block + rfid_control.chinese_block_num );
			card_res_flag = CARD_FLAG_WAIT;
			rfid_control.wait_tick = now_ms();   /* 记录等待起点（真实时间差超时） */
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
			/* 真实时间差超时（不依赖 vTaskDelay 节拍，避免 100Hz 下漂移 10 倍）。
			 * 语义：等 20ms 无响应 → 重发；重发 2 次仍无响应 → 放弃（初始+2 重发=3 次命令） */
			if( (now_ms() - rfid_control.wait_tick) >= (uint32_t)RFID_READ_TIMEOUT_MS )
			{
				if( rfid_control.wait_resend_times >= 2 )
				{
					rfid_control.wait_resend_times = 0;
					card_res_flag = CARD_FLAG_NONE;   /* 放弃 */
				}
				else
				{
					rfid_control.wait_resend_times++;
					card_res_flag = CARD_FLAG_EXIST;  /* 重发 */
				}
			}
		}

		if( card_res_flag == CARD_FLAG_NONE )                /* 无卡：低频轮询读卡号（防失联） */
		{
			BufClear( rfid_control.chinese_data );
			rfid_control.chinese_block_num = 0;
			if( !Motor_IsBusyForLed() ) LED_Sta( 0 );  /* 电机停车期间不碰 LED */
			rfid_control.wait_resend_times = 0;
			static uint32_t none_poll_tick = 0;
			if( (now_ms() - none_poll_tick) >= 200UL )       /* 每 200ms 探测一次，不依赖模块自动上报 */
			{
				none_poll_tick = now_ms();
				Card_ReadCard();
			}
		}
		else if( card_res_flag == CARD_FLAG_LEDLIGHT )       /* LED 保持：轮询读卡号 */
		{
			if( !Motor_IsBusyForLed() ) LED_Sta( 1 );  /* 电机停车期间不碰 LED */
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
					if( !Motor_IsBusyForLed() ) LED_Sta( 0 );  /* 电机停车期间不碰 LED */
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
	rfid_control.wait_tick = 0;
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
	/* 尝试 UTF-8 显示（GBK→UTF8 查找表） */
	{
		static const struct { uint8_t gbk[4]; uint8_t len; const char *utf8; } map[] = {
			{ {0xCC,0xAB,0xD1,0xF4}, 4, "\xe5\xa4\xaa\xe9\x98\xb3" },  /* 太阳 */
			{ {0xB5,0xD8,0xC7,0xF2}, 4, "\xe5\x9c\xb0\xe7\x90\x83" },  /* 地球 */
		};
		for( int i = 0; i < (int)(sizeof(map)/sizeof(map[0])); i++ )
		{
			if( rfid_control.chinese_data[0] == map[i].gbk[0] &&
				rfid_control.chinese_data[1] == map[i].gbk[1] )
			{
				Dbg_Printf(" = %s", map[i].utf8);
				break;
			}
		}
	}
	Dbg_Printf("\r\n");
#endif

	if( ev & RFID_EV_TRIGGER_STOP )
	{
		s_trig_pending = 1;                 /* 先亮绿确认，延时后再停车 */
		s_trig_pending_tick = now_ms();
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
