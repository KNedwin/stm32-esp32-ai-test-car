#ifndef __NVS_PARAMS_H
#define __NVS_PARAMS_H

#include <stdint.h>
#include <stddef.h>

/* ============ 运行时可调参数（网页配置 → NVS 持久化） ============ */
/* 网页单位统一秒，存储统一 ms。默认值 = 原 config.h 宏。 */

#define PARAM_SLOWWIN_MAX   8
#define PARAM_RULE_MAX      8
#define PARAM_RULE_WORD_LEN 16

typedef struct {
    uint32_t start_ms;               /* 第几毫秒开始降速 */
    uint32_t dur_ms;                 /* 持续毫秒 */
    uint8_t  pct;                    /* 降到目标速度百分比 5~95 */
} param_slowwin_t;

typedef struct {
    uint8_t  word[PARAM_RULE_WORD_LEN]; /* GBK 字节 */
    uint8_t  len;                       /* 有效字节数 1~16 */
    uint8_t  count_req;                 /* 触发所需次数 1~10 */
    uint8_t  speak_en;                  /* 触发时播报 */
} param_rule_t;

typedef struct {
    /* 电机 */
    uint32_t late_ms;
    uint32_t slow_ms;
    uint16_t target_speed;
    uint8_t  motor_dir;              /* 0=正转 1=反转 */
    /* 减速窗口列表（多段减速） */
    param_slowwin_t slowwins[PARAM_SLOWWIN_MAX];
    uint8_t  slowwin_count;
    /* 触发词 */
    param_rule_t rules[PARAM_RULE_MAX];
    uint8_t  rule_count;
    uint32_t count_interval_ms;
    uint32_t stop_ramp_ms;           /* H */
    uint32_t wait_ms;                /* I */
    /* 播报与 LED */
    uint32_t led_on_ms;              /* C */
    uint32_t dedup_ms;               /* D */
    uint32_t rfid_poll_ms;
} params_t;

extern params_t g_params;

/* 上电调用一次：内部完成 nvs_flash_init + 读 NVS（无值用默认）+ sanitize */
void params_init(void);

/* 范围校验并钳制到合法区间（网页保存前也可复用），返回 0=原本就合法 */
int params_sanitize(params_t *p);

/* 将当前 g_params 写入 NVS（内部先 sanitize）*/
int params_save(void);

#endif /* __NVS_PARAMS_H */
