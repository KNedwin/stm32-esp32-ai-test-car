#include "rfid_logic.h"
#include <string.h>

/* 触发词规则表默认源（config.h 定义，此处实例化） */
static const trigger_rule_t g_trigger_rules[] = TRIGGER_RULES;

/* ============ 运行时规则配置（Setter 注入，默认=宏表） ============ */
static rfid_rule_rt_t s_rules[TRIGGER_RULES_MAX];
static uint8_t  s_rule_count = 0;
static uint32_t s_dedup_ms    = SPEAK_DEDUP_TIME_S * 1000UL;
static uint32_t s_interval_ms = TRIGGER_COUNT_INTERVAL_MS;
static uint8_t  s_cfg_ready = 0;

static void ensure_defaults(void)
{
    uint8_t i;
    if (s_cfg_ready) return;
    s_rule_count = (uint8_t)(sizeof(g_trigger_rules) / sizeof(g_trigger_rules[0]));
    if (s_rule_count > TRIGGER_RULES_MAX) s_rule_count = TRIGGER_RULES_MAX;
    for (i = 0; i < s_rule_count; i++)
    {
        memset(s_rules[i].word, 0, sizeof(s_rules[i].word));
        memcpy(s_rules[i].word, g_trigger_rules[i].word, g_trigger_rules[i].len);
        s_rules[i].len      = g_trigger_rules[i].len;
        s_rules[i].count_req = g_trigger_rules[i].count_req;
        s_rules[i].speak_en  = g_trigger_rules[i].speak_en;
    }
    s_cfg_ready = 1;
}

void RfidLogic_SetConfig(const rfid_rule_rt_t *rules, uint8_t count,
                         uint32_t dedup_ms, uint32_t interval_ms)
{
    uint8_t i;
    if (count > TRIGGER_RULES_MAX) count = TRIGGER_RULES_MAX;
    for (i = 0; i < count; i++) s_rules[i] = rules[i];
    s_rule_count  = count;
    s_dedup_ms    = dedup_ms;
    s_interval_ms = interval_ms;
    s_cfg_ready   = 1;
}

uint8_t RfidLogic_RuleCount(void)
{
    ensure_defaults();
    return s_rule_count;
}

void RfidLogic_Init(rfid_logic_t *lx)
{
    memset(lx, 0, sizeof(rfid_logic_t));
}

int16_t RfidLogic_TriggerMatch(const uint8_t *data, uint8_t len)
{
    uint8_t  i, r, k;
    uint8_t  dlen;
    uint8_t  num = RfidLogic_RuleCount();

    for( r = 0; r < num; r++ )
    {
        dlen = s_rules[r].len;
        if( dlen == 0 || dlen > len ) continue;   /* 空词条/超长词条防御 */
        for( i = 0; i + dlen <= len; i++ )
        {
            for( k = 0; k < dlen; k++ )
            {
                if( data[i+k] != s_rules[r].word[k] ) break;
            }
            if( k == dlen )
            {
                return (int16_t)r;
            }
        }
    }
    return -1;
}

uint8_t RfidLogic_IsDup(rfid_logic_t *lx, const uint8_t *data, uint32_t now_ms)
{
    ensure_defaults();
    if( (memcmp(lx->last_speak, data, RFID_BLOCK_SIZE) == 0) &&
        (now_ms - lx->last_speak_tick < s_dedup_ms) )
    {
        return 1;
    }
    return 0;
}

void RfidLogic_UpdateSpeak(rfid_logic_t *lx, const uint8_t *data, uint32_t now_ms)
{
    memcpy(lx->last_speak, data, RFID_BLOCK_SIZE);
    lx->last_speak_tick = now_ms;
}

uint8_t RfidLogic_Process(rfid_logic_t *lx, const uint8_t *data, uint8_t len,
                          uint8_t in_stop_sequence, uint32_t now_ms)
{
    int16_t rule_idx;
    uint8_t ev = RFID_EV_NONE;

    ensure_defaults();

    rule_idx = RfidLogic_TriggerMatch(data, len);

    if( rule_idx >= 0 )
    {
        const rfid_rule_rt_t *rule = &s_rules[rule_idx];

        if( lx->trig_triggered[rule_idx] || in_stop_sequence )
        {
            /* 已触发过 / 停车序列期间：按普通卡处理（去重播报） */
            return RfidLogic_IsDup(lx, data, now_ms) ? RFID_EV_NONE : RFID_EV_SPEAK;
        }

        if( rule->count_req == 1 )
        {
            /* 一次性词：直接触发 */
            lx->trig_triggered[rule_idx] = 1;
            ev = RFID_EV_TRIGGER_STOP;
            if( rule->speak_en ) ev |= RFID_EV_SPEAK_FORCED;
            return ev;
        }

        /* 计数型词：首次数无条件放行；后续要求间隔 ≥ TRIGGER_COUNT_INTERVAL_MS */
        if( (lx->trig_count[rule_idx] == 0) ||
            (now_ms - lx->trig_last_count_tick[rule_idx] >= s_interval_ms) )
        {
            lx->trig_count[rule_idx]++;
            lx->trig_last_count_tick[rule_idx] = now_ms;
        }
        if( lx->trig_count[rule_idx] >= rule->count_req )
        {
            lx->trig_count[rule_idx] = 0;
            lx->trig_triggered[rule_idx] = 1;
            ev = RFID_EV_TRIGGER_STOP;
            if( rule->speak_en ) ev |= RFID_EV_SPEAK_FORCED;
            return ev;
        }
        /* 未达次数：按普通卡处理（去重播报） */
        return RfidLogic_IsDup(lx, data, now_ms) ? RFID_EV_NONE : RFID_EV_SPEAK;
    }

    /* 未命中规则：普通卡 */
    return RfidLogic_IsDup(lx, data, now_ms) ? RFID_EV_NONE : RFID_EV_SPEAK;
}
