#include "rfid_task.h"
#include "motor_control_task.h"
#include "config.h"
#include "BSP_USART.h"
#include "Card.h"
#include "led.h"
#include "Debug.h"
#include "cmsis_os.h"
#include "stdio.h"
#include <string.h>

rfid_control_t rfid_control;

extern UART_HandleTypeDef huart1;
extern uint8_t card_res;
extern volatile uint8_t card_res_flag;
extern volatile uint32_t rfid_last_card_tick;

static const trigger_rule_t g_trigger_rules[] = TRIGGER_RULES;
#define TRIGGER_RULES_NUM (sizeof(g_trigger_rules)/sizeof(g_trigger_rules[0]))

static void RFID_Par_Init(void);
static void RFID_HandleCardData(void);   /* 规则匹配 + 去重 + 播报 */
static uint8_t RFID_TriggerMatch(uint8_t *data, uint8_t len, int16_t *rule_idx);
static void RFID_Speak(const uint8_t *data);   /* 送 TTS 播报 */
static void RFID_SpeakIfNotDup(const uint8_t *data); /* 去重后播报 */
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
    printf("<I>0");     /* 上电提示 + 断电保存 */
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
                memcpy(rfid_control.chinese_data, Cmd.block_data, 16);
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
                if( rfid_control.wait_time >= 20 )           /* 20ms 超时重发 */
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
            if( (HAL_GetTick() - rfid_control.led_tick) >= (uint32_t)RFID_READ_DELAY_MS*1UL )
            {
                /* 播报延时结束，开始轮询读卡号维持 LED */
                static uint32_t poll_tick = 0;
                if( (HAL_GetTick() - poll_tick) >= RFID_LED_POLL_MS )
                {
                    poll_tick = HAL_GetTick();
                    ReadCard();
                }
                /* 卡在场由回调刷新 rfid_last_card_tick；脱离 C 秒后熄灭 */
                if( (HAL_GetTick() - rfid_last_card_tick) >= (uint32_t)LED_ON_TIME_S*1000UL )
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
    memset(rfid_control.last_speak, 0, sizeof(rfid_control.last_speak));
    rfid_control.last_speak_tick = 0;
    memset(rfid_control.trig_count, 0, sizeof(rfid_control.trig_count));
    memset(rfid_control.trig_last_count_tick, 0, sizeof(rfid_control.trig_last_count_tick));
    memset(rfid_control.trig_triggered, 0, sizeof(rfid_control.trig_triggered));
    rfid_last_card_tick = HAL_GetTick();

#if RFID_READ_DATA_WHEN_START
    card_res_flag = CARD_FLAG_EXIST;
#else
    card_res_flag = CARD_FLAG_NONE;
#endif
}

/* 卡数据处理：规则匹配 → 计数/触发 → 去重 → 播报 */
static void RFID_HandleCardData(void)
{
    int16_t rule_idx = -1;
    uint8_t hit = RFID_TriggerMatch(rfid_control.chinese_data, 16, &rule_idx);
    uint8_t speak_forced = 0;

#if DBG_ECHO_RFID
    Dbg_Printf("[RFID] ");
    for( uint8_t i = 0; i < 16 && rfid_control.chinese_data[i]; i++ )
    {
        Dbg_Printf("%02X ", rfid_control.chinese_data[i]);
    }
    Dbg_Printf("\r\n");
#endif

    if( hit )
    {
        const trigger_rule_t *rule = &g_trigger_rules[rule_idx];

        if( rfid_control.trig_triggered[rule_idx] )
        {
            /* 已触发过：按普通卡处理（只播报亮灯） */
            RFID_SpeakIfNotDup(rfid_control.chinese_data);
            return;
        }
        if( Motor_IsInStopSequence() )
        {
            /* 停车序列期间：不计数不触发，按普通卡处理 */
            RFID_SpeakIfNotDup(rfid_control.chinese_data);
            return;
        }

        if( rule->count_req == 1 )
        {
            /* 一次性词：直接触发 */
            rfid_control.trig_triggered[rule_idx] = 1;
            motor_trigger_flag = 1;
            if( rule->speak_en ) speak_forced = 1;
        }
        else
        {
            /* 计数型词：首次数无条件放行；后续要求间隔 ≥ TRIGGER_COUNT_INTERVAL_MS 才算有效计数 */
            if( (rfid_control.trig_count[rule_idx] == 0) ||
                ((HAL_GetTick() - rfid_control.trig_last_count_tick[rule_idx])
                 >= (uint32_t)TRIGGER_COUNT_INTERVAL_MS) )
            {
                rfid_control.trig_count[rule_idx]++;
                rfid_control.trig_last_count_tick[rule_idx] = HAL_GetTick();
            }
            if( rfid_control.trig_count[rule_idx] >= rule->count_req )
            {
                rfid_control.trig_count[rule_idx] = 0;
                rfid_control.trig_triggered[rule_idx] = 1;
                motor_trigger_flag = 1;
                if( rule->speak_en ) speak_forced = 1;
            }
        }

        if( speak_forced )
        {
            /* 触发播报：强制播报（不受去重约束），随后更新去重记录 */
            RFID_Speak(rfid_control.chinese_data);
        }
        else
        {
            RFID_SpeakIfNotDup(rfid_control.chinese_data);
        }
    }
    else
    {
        RFID_SpeakIfNotDup(rfid_control.chinese_data);
    }
}

/* 在 data[len] 中搜索任意触发词（子串匹配），命中返回 1 并给出规则号 */
static uint8_t RFID_TriggerMatch(uint8_t *data, uint8_t len, int16_t *rule_idx)
{
    uint8_t  i, r, k;
    uint8_t  dlen;

    for( r = 0; r < TRIGGER_RULES_NUM; r++ )
    {
        dlen = g_trigger_rules[r].len;
        if( dlen > len ) continue;
        for( i = 0; i + dlen <= len; i++ )
        {
            for( k = 0; k < dlen; k++ )
            {
                if( data[i+k] != g_trigger_rules[r].word[k] ) break;
            }
            if( k == dlen )
            {
                *rule_idx = (int16_t)r;
                return 1;
            }
        }
    }
    return 0;
}

/* 直接送 TTS 播报，并更新去重记录 */
static void RFID_Speak(const uint8_t *data)
{
    Usartx_SendString((uint8_t *)data);
    while( !__HAL_UART_GET_FLAG( &HAL_USARTX, UART_FLAG_TC ) ); /* 等 TTS(USART2) 发送完成 */
    memcpy(rfid_control.last_speak, data, 16);
    rfid_control.last_speak_tick = HAL_GetTick();
}

/* 去重后播报：D 秒内相同内容不重复播报（LED 不受影响） */
static void RFID_SpeakIfNotDup(const uint8_t *data)
{
    if( (memcmp(rfid_control.last_speak, data, 16) == 0) &&
        ((HAL_GetTick() - rfid_control.last_speak_tick) < (uint32_t)SPEAK_DEDUP_TIME_S*1000UL) )
    {
        return;    /* 去重：跳过播报 */
    }
    RFID_Speak(data);
}

/* 清空 0 结尾字符串 */
static void BufClear( uint8_t* buf )
{
    uint8_t k = 0;
    for( k = 0; k < 16 && buf[k] != '\0'; k++ )   /* 加长度上限，防 16 字节全非零越界 */
    {
        buf[k] = 0;
    }
}
