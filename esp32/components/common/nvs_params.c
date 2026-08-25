#include "nvs_params.h"
#include "config.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>

/* ============ 默认值（= 原 config.h 宏，首次烧录零感知） ============ */
static const params_t PARAMS_DEFAULT = {
    .late_ms   = MOTOR_START_LATE_TIME_MS,
    .slow_ms   = MOTOR_START_SLOW_TIME_MS,
    .target_speed = MOTOR_TARGET_SPEED,
    .motor_dir = 0,
    .slowwins  = { { MOTOR_TIME_START_S * 1000UL, MOTOR_TIME_DURATION_S * 1000UL,
                     MOTOR_SPEED_PERCENT } },
    .slowwin_count = 1,
    .rules = {
        { .word = {0xCC,0xAB,0xD1,0xF4}, .len = 4, .count_req = 1, .speak_en = 1 }, /* 太阳 */
        { .word = {0xB5,0xD8,0xC7,0xF2}, .len = 4, .count_req = 2, .speak_en = 1 }, /* 地球 */
    },
    .rule_count     = 2,
    .count_interval_ms = TRIGGER_COUNT_INTERVAL_MS,
    .stop_ramp_ms   = TRIGGER_STOP_RAMP_TIME_S * 1000UL,
    .wait_ms        = TRIGGER_WAIT_TIME_S * 1000UL,
    .led_on_ms      = LED_ON_TIME_S * 1000UL,
    .dedup_ms       = SPEAK_DEDUP_TIME_S * 1000UL,
    .rfid_poll_ms   = RFID_READ_DELAY_MS,
};

params_t g_params;

#define CLAMP_U32(v, lo, hi) ((v) < (lo) ? (lo) : ((v) > (hi) ? (hi) : (v)))

/* 范围校验 + 钳制；返回被修正的字段数（0=原本合法） */
int params_sanitize(params_t *p)
{
    int fixed = 0;
    uint8_t i, j;

    p->late_ms      = CLAMP_U32(p->late_ms, 500, 20000);
    p->slow_ms      = CLAMP_U32(p->slow_ms, 500, 30000);
    if (p->target_speed < 100 || p->target_speed > MOTOR_SPEED_MAX)
        { p->target_speed = (p->target_speed > MOTOR_SPEED_MAX) ? MOTOR_SPEED_MAX : 100; fixed++; }
    if (p->motor_dir > 1) { p->motor_dir = 1; fixed++; }

    /* 减速窗口：数量钳制、逐条钳制、按 start 升序、去重叠（保留先者） */
    if (p->slowwin_count > PARAM_SLOWWIN_MAX) { p->slowwin_count = PARAM_SLOWWIN_MAX; fixed++; }
    for (i = 0; i < p->slowwin_count; i++)
    {
        uint32_t old_start = p->slowwins[i].start_ms;
        uint32_t old_dur   = p->slowwins[i].dur_ms;
        p->slowwins[i].start_ms = CLAMP_U32(p->slowwins[i].start_ms, 0, 3600000UL);
        p->slowwins[i].dur_ms   = CLAMP_U32(p->slowwins[i].dur_ms, 200, 120000UL);
        if (p->slowwins[i].pct < 5 || p->slowwins[i].pct > 95)
            { p->slowwins[i].pct = (p->slowwins[i].pct < 5) ? 5 : 95; fixed++; }
        if (old_start != p->slowwins[i].start_ms || old_dur != p->slowwins[i].dur_ms) fixed++;
    }
    /* 冒泡排序（数量小，代价可忽略） */
    for (i = 0; i + 1 < p->slowwin_count; i++)
        for (j = (uint8_t)(i + 1); j < p->slowwin_count; j++)
            if (p->slowwins[j].start_ms < p->slowwins[i].start_ms)
            {
                param_slowwin_t t = p->slowwins[i];
                p->slowwins[i] = p->slowwins[j];
                p->slowwins[j] = t;
            }
    /* 去重叠 */
    for (i = 0; i + 1 < p->slowwin_count; i++)
    {
        uint32_t end_i = p->slowwins[i].start_ms + p->slowwins[i].dur_ms;
        if (end_i > p->slowwins[i + 1].start_ms)
        {
            for (j = (uint8_t)(i + 1); j + 1 < p->slowwin_count; j++)
                p->slowwins[j] = p->slowwins[j + 1];
            p->slowwin_count--;
            fixed++;
        }
    }

    /* 触发词 */
    if (p->rule_count > PARAM_RULE_MAX) { p->rule_count = PARAM_RULE_MAX; fixed++; }
    for (i = 0; i < p->rule_count; i++)
    {
        if (p->rules[i].len == 0 || p->rules[i].len > PARAM_RULE_WORD_LEN)
            { p->rules[i].len = (p->rules[i].len == 0) ? 1 : PARAM_RULE_WORD_LEN; fixed++; }
        if (p->rules[i].count_req == 0 || p->rules[i].count_req > 10)
            { p->rules[i].count_req = (p->rules[i].count_req == 0) ? 1 : 10; fixed++; }
        if (p->rules[i].speak_en > 1) { p->rules[i].speak_en = 1; fixed++; }
    }
    /* 词去重（相同 GBK 内容保留先者） */
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
    p->rfid_poll_ms      = CLAMP_U32(p->rfid_poll_ms, 100, 5000);

    return fixed;
}

void params_init(void)
{
    esp_err_t err;
    nvs_handle_t h = 0;
    size_t len = sizeof(params_t);

    g_params = PARAMS_DEFAULT;

    err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        nvs_flash_erase();
        err = nvs_flash_init();
    }

    if (nvs_open("evcar", NVS_READONLY, &h) == ESP_OK)
    {
        params_t stored;
        if (nvs_get_blob(h, "params", &stored, &len) == ESP_OK &&
            len == sizeof(params_t))
        {
            g_params = stored;
            params_sanitize(&g_params);
        }
        nvs_close(h);
    }
}

int params_save(void)
{
    nvs_handle_t h;
    esp_err_t err;

    params_sanitize(&g_params);

    err = nvs_open("evcar", NVS_READWRITE, &h);
    if (err != ESP_OK) return (int)err;

    err = nvs_set_blob(h, "params", &g_params, sizeof(g_params));
    if (err == ESP_OK) err = nvs_commit(h);
    nvs_close(h);
    return (int)err;
}
