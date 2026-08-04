/* motor_logic（电机状态机纯逻辑）主机单元测试
 * 编译：gcc -std=c11 -I <esp32>/components/common \
 *          test_motor_logic.c <esp32>/components/common/motor_logic.c
 * 使用真实 motor_logic.c 代码，覆盖 sim_motor.py 的规格场景（实现级验证）。
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "motor_logic.h"

static int g_pass = 0, g_fail = 0;
#define CHECK(cond) do { if(cond) g_pass++; else { g_fail++; printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } } while(0)

#define TARGET  MOTOR_TARGET_SPEED
#define A       MOTOR_START_LATE_TIME_MS
#define B       MOTOR_START_SLOW_TIME_MS

static motor_logic_t m;

static void run_to(uint32_t t_start, uint32_t t_end, uint32_t step_ms)
{
    uint32_t t;
    for( t = t_start; t <= t_end; t += step_ms )
    {
        MotorLogic_Step(&m, 0, t);
    }
}

static void test_normal_timing(void)
{
    MotorLogic_Init(&m, 0, TARGET, 600000);
    CHECK(m.state == MOTOR_STATE_IDLE);
    MotorLogic_Step(&m, 0, 1000);                       /* 晚启动前 */
    CHECK(m.state == MOTOR_STATE_IDLE);
    MotorLogic_Step(&m, 0, A);                          /* t=A 进 RAMPUP */
    CHECK(m.state == MOTOR_STATE_RAMPUP);
    MotorLogic_Step(&m, 0, A + B/2);                    /* 缓启动中点速度≈一半 */
    CHECK(m.speed == TARGET * (B/2) / B);
    MotorLogic_Step(&m, 0, A + B);                      /* 缓启动结束 RUN 全速 */
    CHECK(m.state == MOTOR_STATE_RUN && m.speed == TARGET);
}

static void test_slow_window(void)
{
    MotorLogic_Init(&m, 0, TARGET, 600000);
    run_to(0, 42*1000UL - 1, 100);                          /* 窗口前 */
    CHECK(m.state != MOTOR_STATE_SLOW);
    MotorLogic_Step(&m, 0, 42*1000UL);                  /* 窗口起点 */
    CHECK(m.state == MOTOR_STATE_SLOW);
    CHECK(m.speed == TARGET * MOTOR_SPEED_PERCENT / 100);
    MotorLogic_Step(&m, 0, 47*1000UL - 1);              /* 窗口内保持 */
    CHECK(m.state == MOTOR_STATE_SLOW);
    MotorLogic_Step(&m, 0, 47*1000UL);                  /* 窗口结束恢复 */
    CHECK(m.state == MOTOR_STATE_RUN && m.speed == TARGET);
}

static void test_trigger_stop_sequence(void)
{
    MotorLogic_Init(&m, 0, TARGET, 600000);
    run_to(0, 30*1000UL, 100);                            /* 运行到 30s RUN */
    CHECK(m.state == MOTOR_STATE_RUN);
    MotorLogic_Step(&m, 1, 30*1000UL);                    /* 触发 */
    CHECK(m.state == MOTOR_STATE_STOPPING);
    MotorLogic_Step(&m, 0, 30*1000UL + 1000);            /* 减速 1s */
    CHECK(m.state == MOTOR_STATE_STOPPING && m.speed > 0 && m.speed < TARGET);
    MotorLogic_Step(&m, 0, 32*1000UL);                   /* H=2s 后 WAIT */
    CHECK(m.state == MOTOR_STATE_WAIT && m.speed == 0);
    run_to(32*1000UL, 42*1000UL, 100);                  /* I=10s 后重启缓启动 */
    CHECK(m.state == MOTOR_STATE_RAMPUP);
    run_to(42*1000UL, 44*1000UL, 100);                  /* 重启 2s：无晚启动，速度≈一半 */
    CHECK(m.speed == TARGET * 2000 / B);
    run_to(44*1000UL, 46*1000UL, 100);                  /* 重启 4s 进 RUN */
    CHECK(m.state == MOTOR_STATE_RUN && m.speed == TARGET);
}

static void test_stop_time_absolute(void)
{
    /* stop_time=40s，30s 触发 → 序列 12s → 重启缓启动 4s → 46s 进 RUN 时已超 → STOP */
    MotorLogic_Init(&m, 0, TARGET, 40*1000UL);
    run_to(0, 30*1000UL, 100);
    MotorLogic_Step(&m, 1, 30*1000UL);
    run_to(30*1000UL, 42*1000UL, 100);
    CHECK(m.state == MOTOR_STATE_RAMPUP);                /* 序列完整走完 */
    run_to(42*1000UL, 46*1000UL, 100);
    MotorLogic_Step(&m, 0, 46*1000UL + 10);
    CHECK(m.state == MOTOR_STATE_STOP && m.speed == 0);  /* 绝对计时到时 */
}

static void test_max_run_time(void)
{
    MotorLogic_Init(&m, 0, TARGET, 10UL*1000UL*1000UL);  /* 远大于上限 */
    run_to(0, 1000UL*1000UL, 1000);
    CHECK(m.state == MOTOR_STATE_STOP);
}

static void test_stop_sequence_no_stop_time_check(void)
{
    /* stop_time=33s 落在序列期间（30s 触发，序列 30-42s）：不被中断，重启后进 RUN 才 STOP */
    MotorLogic_Init(&m, 0, TARGET, 33*1000UL);
    run_to(0, 30*1000UL, 100);
    MotorLogic_Step(&m, 1, 30*1000UL);
    run_to(30*1000UL, 40*1000UL, 100);
    CHECK(m.state == MOTOR_STATE_WAIT);                  /* 33s 已过 stop_time 但序列不中断 */
    run_to(40*1000UL, 46*1000UL, 100);
    MotorLogic_Step(&m, 0, 46*1000UL + 10);
    CHECK(m.state == MOTOR_STATE_STOP);
}

static void test_trigger_pending_during_rampup(void)
{
    /* IDLE/RAMPUP 期触发挂起：RAMPUP 结束后进 RUN 时消费 */
    MotorLogic_Init(&m, 0, TARGET, 600000);
    MotorLogic_Step(&m, 0, A);                           /* 进 RAMPUP */
    MotorLogic_Step(&m, 1, A + 1000);                    /* 缓启动期间触发（挂起） */
    CHECK(m.state == MOTOR_STATE_RAMPUP);                /* 不立即停车 */
    MotorLogic_Step(&m, 0, A + B);                       /* 缓启动结束进 RUN（当拍不消费） */
    CHECK(m.state == MOTOR_STATE_RUN);
    MotorLogic_Step(&m, 0, A + B + 1);                   /* 下一拍 RUN 分支消费 pending */
    CHECK(m.state == MOTOR_STATE_STOPPING);
}

static void test_calc_stop_time(void)
{
    CHECK(MotorLogic_CalcStopTime(4095) == STOP_TIME_MIN_MS);     /* 最大采样 → 最短停车 */
    CHECK(MotorLogic_CalcStopTime(0) == STOP_TIME_MAX_MS);        /* 最小采样 → 最长停车 */
    uint32_t mid = MotorLogic_CalcStopTime(2000);
    CHECK(mid > STOP_TIME_MIN_MS && mid < STOP_TIME_MAX_MS);      /* 中间值线性区间 */
}

static void test_state_name(void)
{
    CHECK(strcmp(MotorLogic_StateName(MOTOR_STATE_IDLE), "IDLE") == 0);
    CHECK(strcmp(MotorLogic_StateName(MOTOR_STATE_STOP), "STOP") == 0);
}

int main(void)
{
    printf("=== motor_logic 电机状态机单元测试 ===\n");
    test_normal_timing();
    test_slow_window();
    test_trigger_stop_sequence();
    test_stop_time_absolute();
    test_max_run_time();
    test_stop_sequence_no_stop_time_check();
    test_trigger_pending_during_rampup();
    test_calc_stop_time();
    test_state_name();
    printf("通过 %d，失败 %d\n", g_pass, g_fail);
    return g_fail ? 1 : 0;
}
