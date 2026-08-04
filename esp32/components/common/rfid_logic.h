#ifndef __RFID_LOGIC_H
#define __RFID_LOGIC_H

#include "config.h"
#include <stdint.h>

/* 事件位掩码 */
#define RFID_EV_NONE          0x00  /* 无动作（去重拦截/未命中） */
#define RFID_EV_SPEAK         0x01  /* 正常播报（已通过去重） */
#define RFID_EV_SPEAK_FORCED  0x02  /* 强制播报（触发词触发，跳过去重） */
#define RFID_EV_TRIGGER_STOP  0x04  /* 触发停车 */

/* 触发词/去重状态（由调用方持有，测试可独立实例化） */
typedef struct
{
    uint8_t  trig_count[TRIGGER_RULES_MAX];          /* 计数型词已计次数 */
    uint32_t trig_last_count_tick[TRIGGER_RULES_MAX];/* 上次有效计数时刻 */
    uint8_t  trig_triggered[TRIGGER_RULES_MAX];      /* 已触发标志（上电周期内保持） */
    uint8_t  last_speak[RFID_BLOCK_SIZE];            /* 上次播报内容（去重） */
    uint32_t last_speak_tick;                        /* 上次播报时刻 */
} rfid_logic_t;

/* 触发词规则数（由 TRIGGER_RULES 表 sizeof 推导） */
uint8_t RfidLogic_RuleCount(void);

/* 初始化触发/去重状态（上电调用一次） */
void RfidLogic_Init(rfid_logic_t *lx);

/* 在 data[len] 中搜索任意触发词（子串匹配），命中返回规则号，未命中返回 -1 */
int16_t RfidLogic_TriggerMatch(const uint8_t *data, uint8_t len);

/* 去重判断：D 秒内与上次播报内容相同返回 1 */
uint8_t RfidLogic_IsDup(rfid_logic_t *lx, const uint8_t *data, uint32_t now_ms);

/* 播报完成后调用：更新去重记录 */
void RfidLogic_UpdateSpeak(rfid_logic_t *lx, const uint8_t *data, uint32_t now_ms);

/* 卡数据处理核心决策（纯逻辑）：
 * 规则匹配 → 计数/触发判定（首次数放行、10s 间隔、停车序列不计数）→ 去重
 * 返回事件位掩码（RFID_EV_*）；in_stop_sequence 由调用方传入 */
uint8_t RfidLogic_Process(rfid_logic_t *lx, const uint8_t *data, uint8_t len,
                          uint8_t in_stop_sequence, uint32_t now_ms);

#endif
