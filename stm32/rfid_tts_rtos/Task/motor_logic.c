/* 电机状态机纯逻辑（四版共享：STM32/ESP32 × RTOS/裸机，逐字相同）
 * 时序（绝对计时，start_tick 上电记录永不清零）：
 *   IDLE →(t≥A)→ RAMPUP →(t≥A+B)→ RUN
 *   RUN 内 t∈[E,E+G) → SLOW(降速 F%)，窗口按绝对时间仅出现一次
 *   pending_trigger 消费 → STOPPING(H秒线性减速) → WAIT(I秒静止) → RAMPUP 重新缓启动
 *   t≥stop_time 或 t≥1000s → STOP（停车序列期间不检查）
 */
#include "motor_logic.h"
#include <stddef.h>

/* 时序默认值（宏）+ Setter 注入的运行时覆盖 */
static const motor_timing_t s_default_tm = MOTOR_TIMING_DEFAULT;
static const motor_timing_t *s_tm = &s_default_tm;

void MotorLogic_SetTiming(const motor_timing_t *tm)
{
    s_tm = (tm != NULL) ? tm : &s_default_tm;
}

void MotorLogic_Init(motor_logic_t *lx, uint32_t now_ms, uint16_t target_speed, uint32_t stop_time)
{
    lx->state = MOTOR_STATE_IDLE;
    lx->start_tick = now_ms;
    lx->state_tick = now_ms;
    lx->stop_time = stop_time;
    lx->res = 0;
    lx->target_speed = target_speed;
    lx->speed = 0;
    lx->ramp_start = 0;
    lx->pending_trigger = 0;
    lx->tm = s_tm;
}

uint16_t MotorLogic_Step(motor_logic_t *lx, uint8_t trigger, uint32_t now_ms)
{
    uint32_t t, progress;
    uint16_t ramp_speed;

    lx->pending_trigger |= trigger;   /* 触发请求挂起：IDLE/RAMPUP 期不消费，RUN/SLOW 消费 */

    t = now_ms - lx->start_tick;        /* 绝对时间(ms) */
    progress = now_ms - lx->state_tick; /* 阶段时间(ms) */

    switch( lx->state )
    {
        case MOTOR_STATE_IDLE:
            if( t >= lx->tm->late_ms )                     /* 晚启动结束 */
            {
                lx->state = MOTOR_STATE_RAMPUP;
                lx->state_tick = now_ms;
            }
            break;

        case MOTOR_STATE_RAMPUP:                           /* 缓启动 B 秒 0→target */
            ramp_speed = (uint16_t)((uint32_t)lx->target_speed * progress
                                    / lx->tm->slow_ms);
            if( ramp_speed >= lx->target_speed )
            {
                ramp_speed = lx->target_speed;
                lx->state = MOTOR_STATE_RUN;
                lx->state_tick = now_ms;
            }
            lx->speed = ramp_speed;
            break;

        case MOTOR_STATE_RUN:
        case MOTOR_STATE_SLOW:
            if( lx->pending_trigger )                      /* 触发停车 */
            {
                lx->pending_trigger = 0;
                lx->state = MOTOR_STATE_STOPPING;
                lx->state_tick = now_ms;
                lx->ramp_start = lx->speed;                /* 减速起点 */
                break;
            }
            /* 多段减速窗口：遍历窗口列表，t 落入任一 [start,start+dur) 即 SLOW */
            {
                uint8_t w, hit = 0;
                for( w = 0; w < lx->tm->slowwin_count && !hit; w++ )
                {
                    uint32_t ws = lx->tm->slowwin[w].start_ms;
                    uint32_t we = ws + lx->tm->slowwin[w].dur_ms;
                    if( t >= ws && t < we )
                    {
                        hit = 1;
                        lx->state = MOTOR_STATE_SLOW;
                        lx->speed = (uint16_t)((uint32_t)lx->target_speed
                                               * lx->tm->slowwin[w].pct / 100);
                    }
                }
                if( !hit )
                {
                    lx->state = MOTOR_STATE_RUN;
                    lx->speed = lx->target_speed;
                }
            }
            /* 绝对停车检查（电位器/上限）；停车序列期间不检查（由 STOPPING/WAIT 分支保证） */
            if( (t >= lx->stop_time) || (t >= MOTOR_MAX_RUN_TIME_MS) )
            {
                lx->state = MOTOR_STATE_STOP;
                lx->speed = 0;
            }
            break;

        case MOTOR_STATE_STOPPING:                         /* H 秒线性减速至 0 */
            if( progress >= lx->tm->stop_ramp_ms )
            {
                lx->state = MOTOR_STATE_WAIT;
                lx->state_tick = now_ms;
                lx->speed = 0;
            }
            else
            {
                uint32_t remain = lx->tm->stop_ramp_ms - progress;
                lx->speed = (uint16_t)((uint32_t)lx->ramp_start * remain
                                       / lx->tm->stop_ramp_ms);
            }
            break;

        case MOTOR_STATE_WAIT:                             /* 静止等待 I 秒 */
            if( progress >= lx->tm->wait_ms )
            {
                lx->state = MOTOR_STATE_RAMPUP;            /* 重新缓启动（不再经晚启动） */
                lx->state_tick = now_ms;
            }
            break;

        case MOTOR_STATE_STOP:
        default:
            lx->speed = 0;
            break;
    }

    return lx->speed;
}

uint8_t MotorLogic_IsInStopSequence(const motor_logic_t *lx)
{
    return (lx->state == MOTOR_STATE_STOPPING) ||
           (lx->state == MOTOR_STATE_WAIT);
}

uint32_t MotorLogic_CalcStopTime(uint32_t adc_avg)
{
    int32_t r;

    /* clamp 保护：adc→4095 时原公式整数溢出为负→大数（停车最久），此处钳 0（最短） */
    if( adc_avg >= 4095UL ) r = 0;
    else
    {
        r = 5000 - (int32_t)adc_avg * 1000 / (int32_t)(4095 - adc_avg);
        if( r < 0 ) r = 0;
        if( r > RES_MAX ) r = RES_MAX;
    }

    return STOP_TIME_MIN_MS
        + (uint32_t)r * (STOP_TIME_MAX_MS - STOP_TIME_MIN_MS) / RES_MAX;
}

const char *MotorLogic_StateName(motor_state_t s)
{
    switch( s )
    {
        case MOTOR_STATE_IDLE:     return "IDLE";
        case MOTOR_STATE_RAMPUP:   return "RAMPUP";
        case MOTOR_STATE_RUN:      return "RUN";
        case MOTOR_STATE_SLOW:     return "SLOW";
        case MOTOR_STATE_STOPPING: return "STOPPING";
        case MOTOR_STATE_WAIT:     return "WAIT";
        case MOTOR_STATE_STOP:     return "STOP";
    }
    return "?";
}
