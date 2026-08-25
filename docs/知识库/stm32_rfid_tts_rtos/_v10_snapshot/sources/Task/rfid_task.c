#include "rfid_task.h"
#include "motor_control_task.h"
#include "config.h"
#include "BSP_USART.h"
#include "Card.h"
#include "led.h"
#include "Debug.h"
#include "nvs_params.h"
#include "cmsis_os.h"
#include "stdio.h"
#include <string.h>

rfid_control_t rfid_control;

extern UART_HandleTypeDef huart1;
extern uint8_t card_res;
extern volatile uint8_t card_res_flag;
extern volatile uint32_t rfid_last_card_tick;

static void RFID_Par_Init(void);
static void RFID_HandleCardData(void);   /* 规则匹配 + 去重 + 播报（决策在 rfid_logic） */
static void RFID_Speak(const uint8_t *data);   /* 送 TTS 播报 + 更新去重记录 */
static void BufClear(uint8_t *buf);

/**
 * RFID 任务：读卡 → 规则匹配/去重 → 播报 → LED 保持
 * 状态机流转：EXIST(读块) → WAIT(等响应) → RESDATA(处理)
 *            → LEDLIGHT(亮灯轮询保持) → NONE(卡脱离 C 秒后)
 */
void RFID_Task(void const * argument)
{
    /* USER CODE BEGIN RFID_Task */
    vTaskDelay(500);     /* 等待语音模块初始化 */

#if RFID_SETTING_SPEAK_SPEED
    /* 语速/音量/上电提示设置（断电保存） */
    printf("<S>3");     /* 语速最快 */
    vTaskDelay(80);
    printf("<V>6");     /* 音量 */
    vTaskDelay(80);
    printf("<I>7");     /* 上电提示音7号 + 断电保存（与ESP32版选定一致） */
    vTaskDelay(200);
#endif

    RFID_Par_Init();

    for(;;)
    {
        if( card_res_flag == CARD_FLAG_EXIST )               /* 检测到卡，读块 */
        {
            BufClear( Cmd.block_data );
            ReadBlock( rfid_control.read_block + rfid_control.chinese_block_num );
            while( !__HAL_UART_GET_FLAG( &huart1, UART_FLAG_TC ) );
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
#if RFID_DEBUG_MODE
                printf("UID内无数据");
                while( !__HAL_UART_GET_FLAG( &huart1, UART_FLAG_TC ) );
                vTaskDelay(200);
#endif
            }
            else
            {
                /* 单块数据：统一拷贝到 chinese_data 后处理 */
                memcpy(rfid_control.chinese_data, Cmd.block_data, RFID_BLOCK_SIZE);
                rfid_control.chinese_data[RFID_BLOCK_SIZE-1] = 0;  /* 强制 0 结尾，防播报越界 */
                RFID_HandleCardData();
                card_res_flag = CARD_FLAG_LEDLIGHT;
                rfid_control.led_tick = HAL_GetTick();
                rfid_last_card_tick = HAL_GetTick();   /* 重置卡在场计时起点 */
                /* 播报延时后继续（非阻塞：LEDLIGHT 内判断） */
            }
        }
        else if( card_res_flag == CARD_FLAG_WAIT )           /* 等待模块响应 */
        {
            static uint32_t wait_tick = 0;
            if( (HAL_GetTick() - wait_tick) >= 1UL )         /* 1ms 节拍 */
            {
                wait_tick = HAL_GetTick();
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
            if( (HAL_GetTick() - rfid_control.led_tick) >= (uint32_t)RFID_READ_DELAY_MS )
            {
                /* 播报延时结束，开始轮询读卡号维持 LED */
                static uint32_t poll_tick = 0;
                if( (HAL_GetTick() - poll_tick) >= g_params.rfid_poll_ms )
                {
                    poll_tick = HAL_GetTick();
                    ReadCard();
                }
                /* 卡在场由回调刷新 rfid_last_card_tick；脱离 C 秒后熄灭 */
                if( (HAL_GetTick() - rfid_last_card_tick) >= g_params.led_on_ms )
                {
                    LED_Sta( 0 );
                    card_res_flag = CARD_FLAG_NONE;
                }
            }
        }

        vTaskDelay(1);
    }
    /* USER CODE END RFID_Task */
}

/* 参数初始化 */
static void RFID_Par_Init( void )
{
    rfid_control.chinese_block_num = 0;
    rfid_control.read_block = 4;
    rfid_control.wait_time = 0;
    rfid_control.wait_resend_times = 0;
    rfid_control.led_tick = 0;
    RfidLogic_Init(&rfid_control.logic);
    rfid_last_card_tick = HAL_GetTick();

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
                                   HAL_GetTick());

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
    Usartx_SendString((uint8_t *)data);
    RfidLogic_UpdateSpeak(&rfid_control.logic, data, HAL_GetTick());
}

/* 清空缓冲（清到首个 0，最多 RFID_BLOCK_SIZE 字节） */
static void BufClear( uint8_t* buf )
{
    uint8_t k = 0;
    for( k = 0; k < RFID_BLOCK_SIZE && buf[k] != '\0'; k++ )
    {
        buf[k] = 0;
    }
}
