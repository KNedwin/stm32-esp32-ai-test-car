#include "motor_control_task.h"
#include "cmsis_os.h"
#include "config.h"
#include "nvs_params.h"
#include "PWM.h"
#include "Debug.h"

motor_logic_t motor_control;
volatile uint8_t motor_trigger_flag = 0;

static void Motor_ApplySpeed(uint16_t speed);

/* 电机初始化：自动停车时间取运行时参数（原电位器采样已退役） */
void Motor_Init(void)
{
    Motor_SetDirection(g_params.motor_dir);   /* 应用转向参数 */

    MotorLogic_Init(&motor_control, HAL_GetTick(), g_params.target_speed,
                    g_params.autostop_ms);
    Motor_ApplySpeed(0);

#if DBG_ECHO_MOTOR
    Dbg_Printf("[MOTOR] IDLE speed=0\r\n");
#endif
}

/**
 * 电机任务：每拍喂 motor_logic（纯逻辑状态机）并输出速度。
 * 时序（绝对计时）见 motor_logic.c 头部说明。
 */
void Motor_Control_Task(void const * argument)
{
    Motor_Init();

    for(;;)
    {
        motor_state_t old_state = motor_control.state;
        uint8_t trig = motor_trigger_flag;
        uint16_t spd;

        motor_trigger_flag = 0;             /* 触发已移交逻辑层 pending，此处清零 */
        spd = MotorLogic_Step(&motor_control, trig, HAL_GetTick());
        Motor_ApplySpeed(spd);

        if( motor_control.state != old_state )   /* 状态变化：边沿调试输出 */
        {
#if DBG_ECHO_MOTOR
            Dbg_Printf("[MOTOR] %s speed=%u\r\n",
                       MotorLogic_StateName(motor_control.state), motor_control.speed);
#endif
        }

        osDelay(1);
    }
}

static void Motor_ApplySpeed(uint16_t speed)
{
    if( speed > 999 ) speed = 999;
    Motor_Control(speed);
}

uint8_t Motor_IsInStopSequence(void)
{
    return MotorLogic_IsInStopSequence(&motor_control);
}
