#include "rfid_logic.h"
#include <string.h>

/* 触发词规则表（config.h 定义，此处实例化） */
static const trigger_rule_t g_trigger_rules[] = TRIGGER_RULES;

uint8_t RfidLogic_RuleCount(void)
{
    return (uint8_t)(sizeof(g_trigger_rules) / sizeof(g_trigger_rules[0]));
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
        dlen = g_trigger_rules[r].len;
        if( dlen == 0 || dlen > len ) continue;   /* 空词条/超长词条防御 */
        for( i = 0; i + dlen <= len; i++ )
        {
            for( k = 0; k < dlen; k++ )
            {
                if( data[i+k] != g_trigger_rules[r].word[k] ) break;
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
    if( (memcmp(lx->last_speak, data, RFID_BLOCK_SIZE) == 0) &&
        (now_ms - lx->last_speak_tick < (uint32_t)SPEAK_DEDUP_TIME_S*1000UL) )
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

    rule_idx = RfidLogic_TriggerMatch(data, len);

    if( rule_idx >= 0 )
    {
        const trigger_rule_t *rule = &g_trigger_rules[rule_idx];

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
            (now_ms - lx->trig_last_count_tick[rule_idx] >= (uint32_t)TRIGGER_COUNT_INTERVAL_MS) )
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
