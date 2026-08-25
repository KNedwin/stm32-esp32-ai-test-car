#include "nvs_params.h"
#include "config.h"
#include "motor_logic.h"
#include "rfid_logic.h"
#include "stm32f1xx_hal.h"
#include <string.h>

/* 默认值（= config.h 宏） */
static const params_t PARAMS_DEFAULT = {
    .late_ms   = MOTOR_START_LATE_TIME_MS,
    .slow_ms   = MOTOR_START_SLOW_TIME_MS,
    .target_speed = MOTOR_TARGET_SPEED,
    .motor_dir = 0,
    .slowwins  = { { MOTOR_TIME_START_S * 1000UL, MOTOR_TIME_DURATION_S * 1000UL,
                     MOTOR_SPEED_PERCENT } },
    .slowwin_count = 1,
    .rules = {
        { .word = {0xCC,0xAB,0xD1,0xF4}, .len = 4, .count_req = 1, .speak_en = 1 },
        { .word = {0xB5,0xD8,0xC7,0xF2}, .len = 4, .count_req = 2, .speak_en = 1 },
    },
    .rule_count     = 2,
    .count_interval_ms = TRIGGER_COUNT_INTERVAL_MS,
    .stop_ramp_ms   = TRIGGER_STOP_RAMP_TIME_S * 1000UL,
    .wait_ms        = TRIGGER_WAIT_TIME_S * 1000UL,
    .led_on_ms      = LED_ON_TIME_S * 1000UL,
    .dedup_ms       = SPEAK_DEDUP_TIME_S * 1000UL,
    .rfid_poll_ms   = RFID_LED_POLL_MS, /* LED亮灯期轮询间隔(10ms);800ms系误用播报延时宏 */
    .autostop_ms    = 300000UL,
};

params_t g_params;

/* 将 g_params 注入共享逻辑层（电机时序 + 触发词规则）。
 * 上电 params_init 后调用一次；CLI 修改参数后再次调用即时生效。 */
void params_apply(void)
{
    motor_timing_t tm;
    rfid_rule_rt_t rules_rt[8];
    uint8_t i;

    tm.late_ms       = g_params.late_ms;
    tm.slow_ms       = g_params.slow_ms;
    tm.stop_ramp_ms  = g_params.stop_ramp_ms;
    tm.wait_ms       = g_params.wait_ms;
    tm.slowwin_count = g_params.slowwin_count;
    for( i = 0; i < g_params.slowwin_count && i < MOTOR_SLOWWIN_MAX; i++ )
    {
        tm.slowwin[i].start_ms = g_params.slowwins[i].start_ms;
        tm.slowwin[i].dur_ms   = g_params.slowwins[i].dur_ms;
        tm.slowwin[i].pct      = g_params.slowwins[i].pct;
    }
    MotorLogic_SetTiming(&tm);

    for( i = 0; i < g_params.rule_count && i < 8u; i++ )
    {
        memcpy(rules_rt[i].word, g_params.rules[i].word, RFID_BLOCK_SIZE);
        rules_rt[i].len       = g_params.rules[i].len;
        rules_rt[i].count_req = g_params.rules[i].count_req;
        rules_rt[i].speak_en  = g_params.rules[i].speak_en;
    }
    RfidLogic_SetConfig(rules_rt, g_params.rule_count,
                        g_params.dedup_ms, g_params.count_interval_ms);
}

#define PARAMS_PAGE_ADDR 0x0800FC00UL
#define PARAMS_MAGIC     0xA55A
#define CLAMP_U32(v, lo, hi) ((v) > (hi) ? (hi) : (v))

int params_sanitize(params_t *p)
{
    int fixed = 0;
    uint8_t i, j;

    p->late_ms = CLAMP_U32(p->late_ms, 500, 20000);
    p->slow_ms = CLAMP_U32(p->slow_ms, 500, 30000);
    if (p->target_speed > MOTOR_SPEED_MAX)
        { p->target_speed = MOTOR_SPEED_MAX; fixed++; }
    if (p->motor_dir > 1) { p->motor_dir = 1; fixed++; }

    if (p->slowwin_count > 8) { p->slowwin_count = 8; fixed++; }
    for (i = 0; i < p->slowwin_count; i++)
    {
        p->slowwins[i].start_ms = CLAMP_U32(p->slowwins[i].start_ms, 0, 3600000UL);
        p->slowwins[i].dur_ms   = CLAMP_U32(p->slowwins[i].dur_ms, 200, 120000UL);
        if (p->slowwins[i].pct < 5 || p->slowwins[i].pct > 95)
            { p->slowwins[i].pct = (p->slowwins[i].pct < 5) ? 5 : 95; fixed++; }
    }
    for (i = 0; i + 1 < p->slowwin_count; i++)
        for (j = (uint8_t)(i + 1); j < p->slowwin_count; j++)
            if (p->slowwins[j].start_ms < p->slowwins[i].start_ms)
            { param_slowwin_t t = p->slowwins[i]; p->slowwins[i] = p->slowwins[j]; p->slowwins[j] = t; }
    for (i = 0; i + 1 < p->slowwin_count; i++)
    {
        if (p->slowwins[i].start_ms + p->slowwins[i].dur_ms > p->slowwins[i + 1].start_ms)
        {
            for (j = (uint8_t)(i + 1); j + 1 < p->slowwin_count; j++)
                p->slowwins[j] = p->slowwins[j + 1];
            p->slowwin_count--; fixed++;
        }
    }

    if (p->rule_count > 8) { p->rule_count = 8; fixed++; }
    for (i = 0; i < p->rule_count; i++)
    {
        if (p->rules[i].len == 0 || p->rules[i].len > 16)
            { p->rules[i].len = (p->rules[i].len == 0) ? 1 : 16; fixed++; }
        if (p->rules[i].count_req == 0 || p->rules[i].count_req > 10)
            { p->rules[i].count_req = (p->rules[i].count_req == 0) ? 1 : 10; fixed++; }
        if (p->rules[i].speak_en > 1) { p->rules[i].speak_en = 1; fixed++; }
    }
    for (i = 0; i < p->rule_count; i++)
        for (j = (uint8_t)(i + 1); j < p->rule_count; j++)
            if (p->rules[i].len == p->rules[j].len &&
                memcmp(p->rules[i].word, p->rules[j].word, p->rules[i].len) == 0)
            {
                for (uint8_t k = j; k + 1 < p->rule_count; k++) p->rules[k] = p->rules[k + 1];
                p->rule_count--; fixed++; j--;
            }

    p->count_interval_ms = CLAMP_U32(p->count_interval_ms, 1000, 60000);
    p->stop_ramp_ms      = CLAMP_U32(p->stop_ramp_ms, 500, 15000);
    p->wait_ms           = CLAMP_U32(p->wait_ms, 1000, 120000);
    p->led_on_ms         = CLAMP_U32(p->led_on_ms, 500, 30000);
    p->dedup_ms          = CLAMP_U32(p->dedup_ms, 500, 60000);
    p->rfid_poll_ms      = CLAMP_U32(p->rfid_poll_ms, 10, 5000);
    p->autostop_ms       = CLAMP_U32(p->autostop_ms, 10000, 1000000UL);

    return fixed;
}

/* CRC16-MODBUS */
static uint16_t crc16(const uint8_t *d, size_t n)
{
    uint16_t c = 0xFFFF;
    for (size_t i = 0; i < n; i++)
    {
        c ^= d[i];
        for (int b = 0; b < 8; b++)
            c = (c & 1) ? (c >> 1) ^ 0xA001 : (c >> 1);
    }
    return c;
}

typedef struct {
    uint16_t magic;
    uint16_t crc;        /* 对 p 的 CRC16 */
    params_t p;
} flash_blob_t;

void params_init(void)
{
    const flash_blob_t *fb = (const flash_blob_t *)PARAMS_PAGE_ADDR;
    g_params = PARAMS_DEFAULT;

    if (fb->magic == PARAMS_MAGIC)
    {
        uint16_t c = crc16((const uint8_t *)&fb->p, sizeof(fb->p));
        if (c == fb->crc)
        {
            g_params = fb->p;
            params_sanitize(&g_params);
        }
    }
}

int params_save(void)
{
    flash_blob_t blob;
    blob.magic = PARAMS_MAGIC;
    blob.p     = g_params;
    params_sanitize(&blob.p);
    blob.crc = crc16((const uint8_t *)&blob.p, sizeof(blob.p));

    HAL_StatusTypeDef st;
    HAL_FLASH_Unlock();

    FLASH_EraseInitTypeDef er = {
        .TypeErase   = FLASH_TYPEERASE_PAGES,
        .PageAddress = PARAMS_PAGE_ADDR,
        .NbPages     = 1,
    };
    uint32_t page_err = 0;
    st = HAL_FLASHEx_Erase(&er, &page_err);

    if (st == HAL_OK)
    {
        const uint32_t *src = (const uint32_t *)&blob;
        uint32_t addr = PARAMS_PAGE_ADDR;
        for (size_t i = 0; i < sizeof(blob); i += 4)
        {
            st = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr, src[i / 4]);
            if (st != HAL_OK) break;
            addr += 4;
        }
    }

    HAL_FLASH_Lock();
    return (st == HAL_OK) ? 0 : -1;
}
