/* rfid_logic（触发词匹配/计数/去重）主机单元测试
 * 编译：gcc -std=c11 -I <工程根> -I <工程>/Task \
 *          test_logic.c <工程>/Task/rfid_logic.c
 * 使用真实 rfid_logic.c 代码，验证行为矩阵全部规则。
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "rfid_logic.h"

/* ---- 测试框架 ---- */
static int g_pass = 0, g_fail = 0;
#define CHECK(cond) do { if(cond) g_pass++; else { g_fail++; printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } } while(0)

/* 常用数据：太阳 0xCC AB D1 F4 / 地球 B5 D8 C7 F2 */
static const uint8_t SUN[]   = {0xCC,0xAB,0xD1,0xF4,0x00};
static const uint8_t EARTH[] = {0xB5,0xD8,0xC7,0xF2,0x00};
static const uint8_t OTHER[] = {0xB3,0xF6,0xB7,0xA2,0x00};  /* "出发" */

static void test_rule_count(void)
{
    CHECK(RfidLogic_RuleCount() == 2);
}

static void test_trigger_match(void)
{
    CHECK(RfidLogic_TriggerMatch(SUN, 16) == 0);
    CHECK(RfidLogic_TriggerMatch(EARTH, 16) == 1);
    CHECK(RfidLogic_TriggerMatch(OTHER, 16) == -1);
    /* 任意位置包含：数据后段出现"太阳"也应命中 */
    uint8_t buf[16] = {0xAA,0xBB,0xCC,0xAB,0xD1,0xF4,0xDD};
    CHECK(RfidLogic_TriggerMatch(buf, 16) == 0);
    /* 空规则防御：len=0 */
    CHECK(RfidLogic_TriggerMatch(NULL, 0) == -1);
}

static void test_sun_first_trigger(void)
{
    rfid_logic_t lx; RfidLogic_Init(&lx);
    uint8_t ev = RfidLogic_Process(&lx, SUN, 16, 0, 1000);
    CHECK((ev & RFID_EV_TRIGGER_STOP) != 0);      /* 触发停车 */
    CHECK((ev & RFID_EV_SPEAK_FORCED) != 0);      /* 强制播报 */
    RfidLogic_UpdateSpeak(&lx, SUN, 1000);
    /* 已触发：5 秒内再刷同卡 → 去重拦截（不播报不触发） */
    ev = RfidLogic_Process(&lx, SUN, 16, 0, 5000);
    CHECK((ev & RFID_EV_TRIGGER_STOP) == 0);
    CHECK(ev == RFID_EV_NONE);
}

static void test_sun_second_no_trigger_dedup(void)
{
    rfid_logic_t lx; RfidLogic_Init(&lx);
    RfidLogic_Process(&lx, SUN, 16, 0, 1000);      /* 第1次：触发+播报 */
    RfidLogic_UpdateSpeak(&lx, SUN, 1000);
    /* 5 秒内再刷：去重拦截（不播报不触发） */
    uint8_t ev = RfidLogic_Process(&lx, SUN, 16, 0, 5000);
    CHECK((ev & RFID_EV_TRIGGER_STOP) == 0);
    CHECK(ev == RFID_EV_NONE);
    /* 15 秒后：去重放行，只播报不触发 */
    ev = RfidLogic_Process(&lx, SUN, 16, 0, 16000);
    CHECK((ev & RFID_EV_TRIGGER_STOP) == 0);
    CHECK((ev & RFID_EV_SPEAK) != 0);
}

static void test_earth_count_sequence(void)
{
    rfid_logic_t lx; RfidLogic_Init(&lx);
    /* 第1次（t=3s）：计数1，未达次数 → 普通播报 */
    uint8_t ev = RfidLogic_Process(&lx, EARTH, 16, 0, 3000);
    CHECK((ev & RFID_EV_TRIGGER_STOP) == 0);
    CHECK((ev & RFID_EV_SPEAK) != 0);
    RfidLogic_UpdateSpeak(&lx, EARTH, 3000);
    /* 5 秒后（间隔<10s）再刷：不计数、且去重拦截 */
    ev = RfidLogic_Process(&lx, EARTH, 16, 0, 8000);
    CHECK((ev & RFID_EV_TRIGGER_STOP) == 0);
    CHECK(ev == RFID_EV_NONE);
    /* 15 秒后（距上次计数 12s ≥10s）：第2次计数 → 触发+强制播报 */
    ev = RfidLogic_Process(&lx, EARTH, 16, 0, 15000);
    CHECK((ev & RFID_EV_TRIGGER_STOP) != 0);
    CHECK((ev & RFID_EV_SPEAK_FORCED) != 0);
    RfidLogic_UpdateSpeak(&lx, EARTH, 15000);
    /* 已触发：25s 后再刷 → 只播报 */
    ev = RfidLogic_Process(&lx, EARTH, 16, 0, 25000);
    CHECK((ev & RFID_EV_TRIGGER_STOP) == 0);
    CHECK((ev & RFID_EV_SPEAK) != 0);
}

static void test_earth_first_count_no_interval_block(void)
{
    /* 修复点：首次数无条件放行（开机 3s 首刷即计数） */
    rfid_logic_t lx; RfidLogic_Init(&lx);
    uint8_t ev = RfidLogic_Process(&lx, EARTH, 16, 0, 3000);
    CHECK((ev & RFID_EV_SPEAK) != 0);   /* 计数1并播报（此前 bug：被间隔误挡） */
}

static void test_stop_sequence_no_count(void)
{
    /* 停车序列期间（in_stop_sequence=1）：不计数不触发，按普通卡去重播报 */
    rfid_logic_t lx; RfidLogic_Init(&lx);
    uint8_t ev = RfidLogic_Process(&lx, SUN, 16, 1, 1000);
    CHECK((ev & RFID_EV_TRIGGER_STOP) == 0);
    CHECK((ev & RFID_EV_SPEAK) != 0);
}

static void test_dedup_normal_card(void)
{
    rfid_logic_t lx; RfidLogic_Init(&lx);
    CHECK(RfidLogic_Process(&lx, OTHER, 16, 0, 1000) & RFID_EV_SPEAK);
    RfidLogic_UpdateSpeak(&lx, OTHER, 1000);
    /* 5 秒内同内容 → 去重 */
    CHECK(RfidLogic_Process(&lx, OTHER, 16, 0, 5000) == RFID_EV_NONE);
    /* 不同内容 → 播报 */
    CHECK(RfidLogic_Process(&lx, SUN, 16, 0, 6000) & RFID_EV_TRIGGER_STOP); /* 太阳触发 */
    /* 15 秒后同内容恢复播报 */
    RfidLogic_UpdateSpeak(&lx, SUN, 6000);
    CHECK(RfidLogic_Process(&lx, OTHER, 16, 0, 16000) & RFID_EV_SPEAK);
}

static void test_isolated_instances(void)
{
    /* 两实例互不影响（可测性验证）：a 触发不影响 b，b 首次读太阳仍应触发 */
    rfid_logic_t a, b;
    RfidLogic_Init(&a); RfidLogic_Init(&b);
    CHECK(RfidLogic_Process(&a, SUN, 16, 0, 1000) & RFID_EV_TRIGGER_STOP);
    CHECK(RfidLogic_Process(&b, SUN, 16, 0, 1000) & RFID_EV_TRIGGER_STOP);
    /* a 再次读太阳不再触发（triggered 保持） */
    CHECK(!(RfidLogic_Process(&a, SUN, 16, 0, 2000) & RFID_EV_TRIGGER_STOP));
}

int main(void)
{
    printf("=== rfid_logic 单元测试 ===\n");
    test_rule_count();
    test_trigger_match();
    test_sun_first_trigger();
    test_sun_second_no_trigger_dedup();
    test_earth_count_sequence();
    test_earth_first_count_no_interval_block();
    test_stop_sequence_no_count();
    test_dedup_normal_card();
    test_isolated_instances();
    printf("通过 %d，失败 %d\n", g_pass, g_fail);
    return g_fail ? 1 : 0;
}
