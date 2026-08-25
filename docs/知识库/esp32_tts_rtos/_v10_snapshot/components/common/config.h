#ifndef __CONFIG_H
#define __CONFIG_H

#include <stdint.h>

/* ================= 电机时序（晚启动/缓启动） ================= */
#define MOTOR_START_LATE_TIME_MS    2000    /* A：通电延时(ms)后电机才启动。默认2秒 */
#define MOTOR_START_SLOW_TIME_MS    4000    /* B：缓启动时长(ms)，0→目标速度线性加速。默认4秒 */
#define MOTOR_SPEED_MAX             999     /* 电机速度上限（0~999，999=最高电压输出） */
#define MOTOR_TARGET_SPEED          MOTOR_SPEED_MAX  /* 电机目标速度 */
#define MOTOR_MAX_RUN_TIME_MS       (1000UL*1000UL)  /* 电机运行绝对上限：1000秒 */

/* ================= 定时降速窗口（开机后仅一次） ================= */
#define MOTOR_TIME_START_S          42      /* E：电机运行到第几秒开始降速。默认42秒 */
#define MOTOR_SPEED_PERCENT         50      /* F：降速后速度 = 目标速度 × F%。50=一半速 */
#define MOTOR_TIME_DURATION_S       5       /* G：降速持续几秒后恢复目标速度。默认5秒 */

/* ================= 触发词规则表 =================
 * 每条规则格式：{ GBK编码, 长度, 触发次数, 触发时是否播报 }
 * 修改方法（任意 Python 环境）：
 *   python -c "print(' '.join('\\x%02X' % b for b in '新词'.encode('gbk')))"
 * 得到字节串填入下表；注意同步修改"长度"（2个汉字=4）。
 * 规则可增可删，代码自动按数组 sizeof 计算条数。
 * 语义：
 *   count_req=1（一次性词）：第1次读到即播报+停车；之后整个上电周期只播报亮灯
 *   count_req>1（计数型词）：第 count_req 次读到（两次有效计数间隔≥TRIGGER_COUNT_INTERVAL_MS）
 *                        才播报+停车；触发后同样不再触发
 *   触发播报不受去重约束（强制播报）；停车序列期间不计数不触发；LED 始终照常亮
 * 所有触发词的动作统一为：停车（减速H秒+静止I秒）后重新缓启动
 */
typedef struct {
    const uint8_t *word;      /* GBK 字节序列 */
    uint8_t        len;       /* 字节数 */
    uint8_t        count_req; /* 触发所需次数 */
    uint8_t        speak_en;  /* 触发时是否播报（1=播报） */
} trigger_rule_t;

#define TRIGGER_RULES_MAX   8   /* 触发词规则数上限（状态数组容量，勿超过） */
#define TRIGGER_RULES \
{ \
  { (const uint8_t*)"\xCC\xAB\xD1\xF4", 4, 1, 1 },   /* "太阳"：1次触发，播报+停车 */ \
  { (const uint8_t*)"\xB5\xD8\xC7\xF2", 4, 2, 1 }    /* "地球"：2次触发(间隔≥10s)，播报+停车 */ \
}

#define TRIGGER_COUNT_INTERVAL_MS  (10*1000)  /* 计数型词两次有效计数最小间隔(ms)。默认10秒 */
#define TRIGGER_STOP_RAMP_TIME_S   2          /* H：触发停车减速过程耗时(秒)。默认2秒 */
#define TRIGGER_WAIT_TIME_S        10         /* I：停住后静止等待(秒)。默认10秒 */
#define TRIGGER_ACK_GREEN_MS       500        /* 触发后绿色确认窗(ms)：先亮绿再进入减速 */

/* ================= LED 与播报去重 ================= */
#define RFID_BLOCK_SIZE             16      /* 读卡单块数据字节数（S50 一块=16字节） */
#define LED_ON_TIME_S               3       /* C：卡脱离线圈后，LED延迟熄灭时间(秒)。默认3秒 */
#define SPEAK_DEDUP_TIME_S          10      /* D：相同内容去重窗口(秒)。默认10秒 */
#define RFID_READ_DELAY_MS          800     /* 一次播报后到下次读卡的间隔(ms)。默认800ms */
#define RFID_LED_POLL_MS            10      /* LED亮灯期间轮询读卡的间隔(ms)。默认10ms */
#define RFID_READ_TIMEOUT_MS        20      /* 读块响应超时(ms)，超时重发，2次后放弃 */

/* ================= 电位器自动停止 ================= */
#define RES_MAX                     5000    /* 电位器阻值量程(用于线性插值) */
#define STOP_TIME_MIN_MS            (10*1000)    /* 停车时间下限：10秒 */
#define STOP_TIME_MAX_MS            (10*60*1000) /* 停车时间上限：600秒(10分钟) */

/* ================= 数据输出口（ESP32：USB-Serial-JTAG console） ================= */
#define DBG_USART_ENABLE            1       /* 总开关：1=启用输出 */
#define DBG_ECHO_RFID               1       /* 1=输出读卡数据 */
#define DBG_ECHO_MOTOR              1       /* 1=输出电机状态 */
#define DBG_ECHO_LED                1       /* 1=输出LED状态 */

/* ================= 无硬件演示模式 ================= */
#define DEMO_MODE                   0       /* 1=演示：模拟刷卡 + TTS hex 打印（无需外设）；0=真机 */

/* ================= 板载 RGB LED（WS2812） ================= */
#define LED_WS2812_PIN              48      /* ESP32-S3-DevKitC-1 板载 RGB LED */

/* ================= 其他 ================= */
#define RFID_READ_DATA_WHEN_START   1       /* 1=开机就尝试读卡(默认)；0=等刷卡才读 */
#define RFID_SETTING_SPEAK_SPEED    1       /* 1=开机设置语速/音量/保存(默认) */

#endif /* __CONFIG_H */
