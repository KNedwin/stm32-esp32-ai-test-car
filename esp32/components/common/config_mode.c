#include "config_mode.h"
#include "config.h"
#include "ws2812.h"
#include "wifi_ap.h"
#include "web_server.h"
#include "debug.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_system.h"
#include <stdio.h>

#define CFG_NS              "evcar"
#define CFG_KEY_CNT         "cfg_cnt"
#define CFG_ENTER_THRESHOLD 3      /* 连按3次RST = 3次快速重启 */
#define CFG_CLEAR_AFTER_MS  10000  /* 持续运行10s后计数清零 */

static uint8_t s_cfg_mode = 0;

uint8_t config_mode_should_enter(void)
{
    return s_cfg_mode;
}

static void cfg_count_write(uint8_t v)
{
    nvs_handle_t h;
    if (nvs_open(CFG_NS, NVS_READWRITE, &h) == ESP_OK)
    {
        nvs_set_u8(h, CFG_KEY_CNT, v);
        nvs_commit(h);
        nvs_close(h);
    }
}

/* 延时清零任务：持续运行超过阈值即认为不是配置手势
 * 注意：栈必须足够大——NVS 写入调用链峰值 >1.5KB，过小会栈溢出panic重启 */
static void cfg_clear_task(void *arg)
{
    vTaskDelay(pdMS_TO_TICKS(CFG_CLEAR_AFTER_MS));

    nvs_handle_t h;
    uint8_t cur = 0;
    int wrote = 0;

    if (nvs_open(CFG_NS, NVS_READWRITE, &h) == ESP_OK)
    {
        nvs_get_u8(h, CFG_KEY_CNT, &cur);
        if (cur != 0)
        {
            nvs_set_u8(h, CFG_KEY_CNT, 0);
            nvs_commit(h);
            wrote = 1;
        }
        nvs_close(h);
    }

    printf("[CFG] counter cleared (was %u, wrote=%d)\r\n", cur, wrote);
    vTaskDelete(NULL);
}

void config_mode_boot_check(void)
{
    esp_reset_reason_t r = esp_reset_reason();
    nvs_handle_t h;
    uint8_t cnt = 0;
    uint8_t counted = 1;

    /* 崩溃/看门狗/欠压复位不累计（防程序异常循环误入），且保留原计数 */
    if (r == ESP_RST_PANIC || r == ESP_RST_INT_WDT || r == ESP_RST_TASK_WDT ||
        r == ESP_RST_WDT     || r == ESP_RST_BROWNOUT)
        counted = 0;

    if (nvs_open(CFG_NS, NVS_READONLY, &h) == ESP_OK)
    {
        nvs_get_u8(h, CFG_KEY_CNT, &cnt);
        nvs_close(h);
    }

    printf("[CFG] rst_reason=%d stored_cnt=%d\r\n", (int)r, cnt);

    if (counted) cnt++;

    if (cnt >= CFG_ENTER_THRESHOLD)
    {
        s_cfg_mode = 1;
        cfg_count_write(0);      /* 消费计数 */
        return;
    }
    if (counted) cfg_count_write(cnt);

    /* 诊断：蓝色快闪 cnt 次（未累计则长闪2次表示被过滤） */
    {
        int blinks = counted ? cnt : 0;
        vTaskDelay(pdMS_TO_TICKS(600));
        for (int i = 0; i < blinks; i++)
        {
            WS2812_SetColor(LED_COLOR_BOOT);
            vTaskDelay(pdMS_TO_TICKS(120));
            WS2812_SetColor(LED_COLOR_OFF);
            vTaskDelay(pdMS_TO_TICKS(120));
        }
        if (!counted)
        {
            WS2812_SetColor(LED_COLOR_STOPPED);   /* 红色长亮=本次复位被过滤 */
            vTaskDelay(pdMS_TO_TICKS(1000));
            WS2812_SetColor(LED_COLOR_OFF);
        }
    }

    xTaskCreate(cfg_clear_task, "cfgclr", 4096, NULL, 1, NULL);
}

void config_mode_run(void)
{
    printf("[CFG] enter config mode: WiFi EV-Car-Setup -> http://192.168.4.1\r\n");
    Dbg_Printf("[CFG] config mode\r\n");

    WS2812_SetColor(LED_COLOR_BOOT);       /* 常亮蓝 = 配置模式中 */

    wifi_ap_start();
    web_server_start();

    /* 空闲监控：5 分钟无 HTTP 活动自动重启回正常模式（防呆） */
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(5000));
        if (web_server_idle_seconds() >= 300)
        {
            printf("[CFG] idle timeout, restart\\r\\n");
            esp_restart();
        }
    }
}
