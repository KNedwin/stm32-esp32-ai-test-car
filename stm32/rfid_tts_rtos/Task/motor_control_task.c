#include "motor_control_task.h"
#include "config.h"
#include "BSP_ADC.h"
#include "PWM.h"
#include "cmsis_os.h"
#include "Debug.h"

motor_control_t motor_control;
volatile uint8_t motor_trigger_flag = 0;

static void Motor_ApplySpeed(uint16_t speed);
static void Motor_CalcStopTime(void);

/**
 * 电机控制任务。
 * 时序（绝对计时，start_tick 上电记录永不清零）：
 *   IDLE →(t≥A)→ RAMPUP →(t≥A+B)→ RUN
 *   RUN 内 t∈[E,E+G) → SLOW(降速 F%)，开机仅一次
 *   motor_trigger_flag → STOPPING(H秒线性减速) → WAIT(I秒静止) → RAMPUP 重新缓启动
 *   t≥stop_time 或 t≥1000s → STOP（绝对时间判断）
 */
void Motor_Control_Task(void const * argument)
{
    uint32_t t = 0;
    uint32_t progress = 0;
    uint16_t ramp_speed = 0;

    /* 采样电位器并计算自动停止时间 */
    Motor_CalcStopTime();

    motor_control.state = MOTOR_STATE_IDLE;
    motor_control.target_speed = MOTOR_TARGET_SPEED;
    motor_control.speed = 0;
    motor_control.start_tick = HAL_GetTick();
    motor_control.state_tick = motor_control.start_tick;
    Motor_ApplySpeed(0);

#if DBG_ECHO_MOTOR
    Dbg_Printf("[MOTOR] IDLE speed=0\r\n");
#endif

    for(;;)
    {
        t = HAL_GetTick() - motor_control.start_tick;   /* 绝对时间(ms) */
        progress = HAL_GetTick() - motor_control.state_tick; /* 阶段时间(ms) */

        switch( motor_control.state )
        {
            case MOTOR_STATE_IDLE:
                if( t >= MOTOR_START_LATE_TIME_MS )     /* 晚启动结束 */
                {
                    motor_control.state = MOTOR_STATE_RAMPUP;
                    motor_control.state_tick = HAL_GetTick();
#if DBG_ECHO_MOTOR
                    Dbg_Printf("[MOTOR] RAMPUP speed=%u\r\n", motor_control.speed);
#endif
                }
                break;

            case MOTOR_STATE_RAMPUP:                    /* 缓启动 B 秒 0→target */
                ramp_speed = (uint16_t)((uint32_t)motor_control.target_speed * progress
                                        / MOTOR_START_SLOW_TIME_MS);
                if( ramp_speed >= motor_control.target_speed )
                {
                    ramp_speed = motor_control.target_speed;
                    motor_control.state = MOTOR_STATE_RUN;
                    motor_control.state_tick = HAL_GetTick();
#if DBG_ECHO_MOTOR
                    Dbg_Printf("[MOTOR] RUN speed=%u\r\n", ramp_speed);
#endif
                }
                Motor_ApplySpeed(ramp_speed);
                break;

            case MOTOR_STATE_RUN:
            case MOTOR_STATE_SLOW:
                if( motor_trigger_flag )                /* 触发停车 */
                {
                    motor_trigger_flag = 0;
                    motor_control.state = MOTOR_STATE_STOPPING;
                    motor_control.state_tick = HAL_GetTick();
                    motor_control.ramp_start = motor_control.speed;  /* 减速起点 */
#if DBG_ECHO_MOTOR
                    Dbg_Printf("[MOTOR] STOPPING speed=%u\r\n", motor_control.speed);
#endif
                    break;
                }
                /* 降速窗口：绝对时间 t∈[E,E+G) 秒，开机仅一次 */
                if( (t >= (uint32_t)MOTOR_TIME_START_S*1000) &&
                    (t < (uint32_t)(MOTOR_TIME_START_S+MOTOR_TIME_DURATION_S)*1000) )
                {
                    if( motor_control.state != MOTOR_STATE_SLOW )
                    {
                        motor_control.state = MOTOR_STATE_SLOW;
#if DBG_ECHO_MOTOR
                        Dbg_Printf("[MOTOR] SLOW speed=%u\r\n",
                                   (uint16_t)(motor_control.target_speed*MOTOR_SPEED_PERCENT/100));
#endif
                    }
                    Motor_ApplySpeed((uint16_t)(motor_control.target_speed*MOTOR_SPEED_PERCENT/100));
                }
                else
                {
                    if( motor_control.state != MOTOR_STATE_RUN )
                    {
                        motor_control.state = MOTOR_STATE_RUN;
#if DBG_ECHO_MOTOR
                        Dbg_Printf("[MOTOR] RUN speed=%u\r\n", motor_control.target_speed);
#endif
                    }
                    Motor_ApplySpeed(motor_control.target_speed);
                }
                /* 绝对停车检查（电位器/上限） */
                if( (t >= motor_control.stop_time) || (t >= MOTOR_MAX_RUN_TIME_MS) )
                {
                    motor_control.state = MOTOR_STATE_STOP;
                    Motor_ApplySpeed(0);
#if DBG_ECHO_MOTOR
                    Dbg_Printf("[MOTOR] STOP speed=0\r\n");
#endif
                }
                break;

            case MOTOR_STATE_STOPPING:                  /* H 秒线性减速至 0 */
                if( progress >= (uint32_t)TRIGGER_STOP_RAMP_TIME_S*1000UL )
                {
                    motor_control.state = MOTOR_STATE_WAIT;
                    motor_control.state_tick = HAL_GetTick();
                    Motor_ApplySpeed(0);
#if DBG_ECHO_MOTOR
                    Dbg_Printf("[MOTOR] WAIT speed=0\r\n");
#endif
                }
                else
                {
                    /* 从 ramp_start 线性衰减到 0（整数安全） */
                    uint32_t remain = (uint32_t)TRIGGER_STOP_RAMP_TIME_S*1000UL - progress;
                    Motor_ApplySpeed((uint16_t)((uint32_t)motor_control.ramp_start
                                                * remain
                                                / ((uint32_t)TRIGGER_STOP_RAMP_TIME_S*1000UL)));
                }
                break;

            case MOTOR_STATE_WAIT:                      /* 静止等待 I 秒 */
                if( progress >= (uint32_t)TRIGGER_WAIT_TIME_S*1000UL )
                {
                    motor_control.state = MOTOR_STATE_RAMPUP;
                    motor_control.state_tick = HAL_GetTick();
#if DBG_ECHO_MOTOR
                    Dbg_Printf("[MOTOR] RAMPUP speed=0\r\n");
#endif
                }
                break;

            case MOTOR_STATE_STOP:
            default:
                Motor_ApplySpeed(0);
                break;
        }

        osDelay(1);
    }
}

/* 输出当前速度到 PWM（0 → 双路 999 停机） */
static void Motor_ApplySpeed(uint16_t speed)
{
    if( speed > 999 ) speed = 999;
    motor_control.speed = speed;
    Motor_Control(speed);
}

/* 电位器换算停车时间（10s~600s 线性插值，含边界保护） */
static void Motor_CalcStopTime(void)
{
    uint32_t adc_value = 0;
    int32_t  r;
    uint8_t  i;

    for( i = 0; i < 20; i++ )
    {
        adc_value += Get_ADC_Value();
        osDelay(1);
    }
    adc_value /= 20;

    /* 原案例公式在 adc→4095 时整数溢出为负，此处 clamp 保护：
     * adc=4095 → res=0 → 停车时间最短；adc=0 → res=RES_MAX → 最长 */
    if( adc_value >= 4095UL ) r = 0;
    else
    {
        r = 5000 - (int32_t)adc_value * 1000 / (int32_t)(4095 - adc_value);
        if( r < 0 ) r = 0;
        if( r > RES_MAX ) r = RES_MAX;
    }

    motor_control.res = (uint32_t)r;
    motor_control.stop_time = STOP_TIME_MIN_MS
        + (uint32_t)r * (STOP_TIME_MAX_MS - STOP_TIME_MIN_MS) / RES_MAX;
}

/* 是否处于停车序列（供 rfid 判断不计数不触发） */
uint8_t Motor_IsInStopSequence(void)
{
    return (motor_control.state == MOTOR_STATE_STOPPING) ||
           (motor_control.state == MOTOR_STATE_WAIT);
}
