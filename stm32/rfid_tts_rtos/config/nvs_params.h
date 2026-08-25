#ifndef __NVS_PARAMS_H
#define __NVS_PARAMS_H

#include <stdint.h>
#include <stddef.h>

/* 参数结构与字段与 ESP32 nvs_params.h 完全同构；
 * 存储介质不同：内部 Flash 末页(0x0800FC00, 1KB, CRC 校验)。 */

typedef struct {
    uint32_t start_ms;
    uint32_t dur_ms;
    uint8_t  pct;
} param_slowwin_t;

typedef struct {
    uint8_t  word[16];
    uint8_t  len;
    uint8_t  count_req;
    uint8_t  speak_en;
} param_rule_t;

typedef struct {
    uint32_t late_ms;
    uint32_t slow_ms;
    uint16_t target_speed;
    uint8_t  motor_dir;
    param_slowwin_t slowwins[8];
    uint8_t  slowwin_count;
    param_rule_t rules[8];
    uint8_t  rule_count;
    uint32_t count_interval_ms;
    uint32_t stop_ramp_ms;
    uint32_t wait_ms;
    uint32_t led_on_ms;
    uint32_t dedup_ms;
    uint32_t rfid_poll_ms;
    uint32_t autostop_ms;
} params_t;

extern params_t g_params;

void params_init(void);              /* 读末页+CRC，失败回落默认 */
int  params_save(void);              /* 擦页+写入+CRC，返回0=成功 */
void params_apply(void);             /* g_params 注入逻辑层（上电/CLI改后调用） */
void params_apply(void);             /* g_params 注入逻辑层（上电/CLI改后调用） */
int  params_sanitize(params_t *p);   /* 范围钳制 */

#endif /* __NVS_PARAMS_H */
