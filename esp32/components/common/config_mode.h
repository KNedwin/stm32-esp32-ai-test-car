#ifndef __CONFIG_MODE_H
#define __CONFIG_MODE_H

#include <stdint.h>

/* 进入配置模式检测：
 * 手势 = 快速连按 3 次 RST（或等效快速通断电）
 * 实现 = 每次上电 NVS 计数 +1；持续运行 10 秒自动清零；
 *        崩溃/看门狗/欠压复位不累计；快速重启累计达 3 次（CFG_ENTER_THRESHOLD）进入。 */

/* 上电调用一次（须在 params_init 之后）：
 * 累计计数并判定是否应进入配置模式 */
void config_mode_boot_check(void);

/* 是否应进入配置模式（boot_check 之后有效） */
uint8_t config_mode_should_enter(void);

/* 进入配置模式：阶段三=蓝色快闪占位循环（永不返回）；
 * 阶段四将替换为 启动WiFi AP + HTTP 服务，完成后 esp_restart() */
void config_mode_run(void);

#endif /* __CONFIG_MODE_H */
