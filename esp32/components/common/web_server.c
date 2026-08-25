#include "web_server.h"
#include "nvs_params.h"
#include "gbk_utf8.h"
#include "esp_http_server.h"
#include "esp_timer.h"
#include "cJSON.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

extern const unsigned char webpage_start[] asm("_binary_webpage_html_start");
extern const unsigned char webpage_end[]   asm("_binary_webpage_html_end");

static httpd_handle_t s_server = NULL;
static int64_t s_last_activity_us = 0;

static void touch(void) { s_last_activity_us = esp_timer_get_time(); }

int web_server_idle_seconds(void)
{
    return (int)((esp_timer_get_time() - s_last_activity_us) / 1000000LL);
}

/* ---------- GET / : 配置页 ---------- */
static esp_err_t page_handler(httpd_req_t *req)
{
    touch();
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    size_t len = webpage_end - webpage_start;
    return httpd_resp_send(req, (const char *)webpage_start, len);
}

/* ---------- 构造参数 JSON ---------- */
static char *build_params_json(void)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "late_ms",        (double)g_params.late_ms);
    cJSON_AddNumberToObject(root, "slow_ms",        (double)g_params.slow_ms);
    cJSON_AddNumberToObject(root, "target_speed",   g_params.target_speed);
    cJSON_AddNumberToObject(root, "motor_dir",      g_params.motor_dir);
    cJSON_AddNumberToObject(root, "stop_ramp_ms",   (double)g_params.stop_ramp_ms);
    cJSON_AddNumberToObject(root, "wait_ms",        (double)g_params.wait_ms);
    cJSON_AddNumberToObject(root, "led_on_ms",      (double)g_params.led_on_ms);
    cJSON_AddNumberToObject(root, "dedup_ms",       (double)g_params.dedup_ms);
    cJSON_AddNumberToObject(root, "rfid_poll_ms",   (double)g_params.rfid_poll_ms);
    cJSON_AddNumberToObject(root, "count_interval_ms", (double)g_params.count_interval_ms);
    cJSON_AddNumberToObject(root, "autostop_ms",    (double)g_params.autostop_ms);

    cJSON *wins = cJSON_AddArrayToObject(root, "slowwins");
    for (int i = 0; i < g_params.slowwin_count; i++)
    {
        cJSON *w = cJSON_CreateObject();
        cJSON_AddNumberToObject(w, "start_ms", (double)g_params.slowwins[i].start_ms);
        cJSON_AddNumberToObject(w, "dur_ms",   (double)g_params.slowwins[i].dur_ms);
        cJSON_AddNumberToObject(w, "pct",      g_params.slowwins[i].pct);
        cJSON_AddItemToArray(wins, w);
    }

    cJSON *rules = cJSON_AddArrayToObject(root, "rules");
    for (int i = 0; i < g_params.rule_count; i++)
    {
        char ubuf[48];
        cJSON *r = cJSON_CreateObject();
        if (gbk_to_utf8(g_params.rules[i].word, g_params.rules[i].len,
                        ubuf, sizeof(ubuf)) > 0)
            cJSON_AddStringToObject(r, "text", ubuf);
        else
            cJSON_AddStringToObject(r, "text", "");
        cJSON_AddNumberToObject(r, "count", g_params.rules[i].count_req);
        cJSON_AddNumberToObject(r, "speak", g_params.rules[i].speak_en);
        cJSON_AddItemToArray(rules, r);
    }

    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return json;   /* 调用方 free */
}

static esp_err_t get_params_handler(httpd_req_t *req)
{
    touch();
    char *json = build_params_json();
    httpd_resp_set_type(req, "application/json");
    esp_err_t err = httpd_resp_send(req, json, strlen(json));
    free(json);
    return err;
}

/* ---------- POST /api/params ---------- */
static void apply_json_to_params(cJSON *root)
{
    cJSON *it;

    if ((it = cJSON_GetObjectItem(root, "late_s")) && cJSON_IsNumber(it))
        g_params.late_ms = (uint32_t)(it->valuedouble * 1000);
    if ((it = cJSON_GetObjectItem(root, "slow_s")) && cJSON_IsNumber(it))
        g_params.slow_ms = (uint32_t)(it->valuedouble * 1000);
    if ((it = cJSON_GetObjectItem(root, "target_speed")) && cJSON_IsNumber(it))
        g_params.target_speed = (uint16_t)it->valuedouble;
    if ((it = cJSON_GetObjectItem(root, "motor_dir")) && cJSON_IsNumber(it))
        g_params.motor_dir = (uint8_t)it->valuedouble;
    if ((it = cJSON_GetObjectItem(root, "stop_ramp_s")) && cJSON_IsNumber(it))
        g_params.stop_ramp_ms = (uint32_t)(it->valuedouble * 1000);
    if ((it = cJSON_GetObjectItem(root, "wait_s")) && cJSON_IsNumber(it))
        g_params.wait_ms = (uint32_t)(it->valuedouble * 1000);
    if ((it = cJSON_GetObjectItem(root, "led_on_s")) && cJSON_IsNumber(it))
        g_params.led_on_ms = (uint32_t)(it->valuedouble * 1000);
    if ((it = cJSON_GetObjectItem(root, "dedup_s")) && cJSON_IsNumber(it))
        g_params.dedup_ms = (uint32_t)(it->valuedouble * 1000);
    if ((it = cJSON_GetObjectItem(root, "poll_s")) && cJSON_IsNumber(it))
        g_params.rfid_poll_ms = (uint32_t)(it->valuedouble * 1000);
    if ((it = cJSON_GetObjectItem(root, "count_interval_s")) && cJSON_IsNumber(it))
        g_params.count_interval_ms = (uint32_t)(it->valuedouble * 1000);
    if ((it = cJSON_GetObjectItem(root, "autostop_s")) && cJSON_IsNumber(it))
        g_params.autostop_ms = (uint32_t)(it->valuedouble * 1000);

    if ((it = cJSON_GetObjectItem(root, "slowwins")) && cJSON_IsArray(it))
    {
        int n = cJSON_GetArraySize(it);
        if (n > PARAM_SLOWWIN_MAX) n = PARAM_SLOWWIN_MAX;
        g_params.slowwin_count = (uint8_t)n;
        for (int i = 0; i < n; i++)
        {
            cJSON *w = cJSON_GetArrayItem(it, i);
            cJSON *a = cJSON_GetObjectItem(w, "start_s");
            cJSON *b = cJSON_GetObjectItem(w, "dur_s");
            cJSON *c = cJSON_GetObjectItem(w, "pct");
            g_params.slowwins[i].start_ms = (a && cJSON_IsNumber(a))
                ? (uint32_t)(a->valuedouble * 1000) : 0;
            g_params.slowwins[i].dur_ms = (b && cJSON_IsNumber(b))
                ? (uint32_t)(b->valuedouble * 1000) : 1000;
            g_params.slowwins[i].pct = (c && cJSON_IsNumber(c)) ? (uint8_t)c->valuedouble : 50;
        }
    }

    if ((it = cJSON_GetObjectItem(root, "rules")) && cJSON_IsArray(it))
    {
        int n = cJSON_GetArraySize(it);
        if (n > PARAM_RULE_MAX) n = PARAM_RULE_MAX;
        g_params.rule_count = (uint8_t)n;
        for (int i = 0; i < n; i++)
        {
            cJSON *r = cJSON_GetArrayItem(it, i);
            cJSON *tx  = cJSON_GetObjectItem(r, "text");
            cJSON *cnt = cJSON_GetObjectItem(r, "count");
            cJSON *spk = cJSON_GetObjectItem(r, "speak");

            memset(g_params.rules[i].word, 0, PARAM_RULE_WORD_LEN);
            const char *text = (tx && cJSON_IsString(tx)) ? tx->valuestring : "";
            uint8_t gb[PARAM_RULE_WORD_LEN];
            int glen = utf8_to_gbk(text, strlen(text), gb, sizeof(gb));
            if (glen < 0) glen = 0;
            if (glen > PARAM_RULE_WORD_LEN) glen = PARAM_RULE_WORD_LEN;
            memcpy(g_params.rules[i].word, gb, (size_t)glen);
            g_params.rules[i].len = (uint8_t)((glen == 0) ? 1 : glen);

            g_params.rules[i].count_req = (cnt && cJSON_IsNumber(cnt))
                ? (uint8_t)cnt->valuedouble : 1;
            g_params.rules[i].speak_en = (spk && cJSON_IsNumber(spk))
                ? (uint8_t)spk->valuedouble : 1;
        }
    }
}

static esp_err_t post_params_handler(httpd_req_t *req)
{
    touch();
    int total = req->content_len;
    if (total <= 0 || total > 4096)
    {
        httpd_resp_set_status(req, "400 Bad Request");
        return httpd_resp_send(req, "{\"err\":\"bad length\"}", HTTPD_RESP_USE_STRLEN);
    }

    char *buf = malloc(total + 1);
    if (!buf) { httpd_resp_set_status(req, "500"); return httpd_resp_send(req, "{\"err\":\"mem\"}", HTTPD_RESP_USE_STRLEN); }

    int rec = 0;
    while (rec < total)
    {
        int r = httpd_req_recv(req, buf + rec, total - rec);
        if (r <= 0) { free(buf); httpd_resp_set_status(req, "500"); return httpd_resp_send(req, "{\"err\":\"recv\"}", HTTPD_RESP_USE_STRLEN); }
        rec += r;
    }
    buf[total] = '\0';

    cJSON *root = cJSON_Parse(buf);
    if (!root)
    {
        free(buf);
        httpd_resp_set_type(req, "application/json");
        return httpd_resp_send(req, "{\"ok\":false,\"err\":\"json\"}", HTTPD_RESP_USE_STRLEN);
    }
    free(buf);

    apply_json_to_params(root);
    cJSON_Delete(root);

    params_save();   /* sanitize + NVS */

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"ok\":true,\"msg\":\"saved, restarting\"}", HTTPD_RESP_USE_STRLEN);
    vTaskDelay(pdMS_TO_TICKS(500));
    esp_restart();
    return ESP_OK;   /* unreachable */
}

static esp_err_t restart_handler(httpd_req_t *req)
{
    touch();
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"ok\":true,\"msg\":\"restarting\"}", HTTPD_RESP_USE_STRLEN);
    vTaskDelay(pdMS_TO_TICKS(600));
    esp_restart();
    return ESP_OK;
}

void web_server_start(void)
{
    if (s_server) return;

    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    cfg.max_uri_handlers = 8;

    if (httpd_start(&s_server, &cfg) != ESP_OK) { s_server = NULL; return; }

    httpd_uri_t u_page   = { .uri="/",           .method=HTTP_GET,  .handler=page_handler };
    httpd_uri_t u_get    = { .uri="/api/params", .method=HTTP_GET,  .handler=get_params_handler };
    httpd_uri_t u_post   = { .uri="/api/params", .method=HTTP_POST, .handler=post_params_handler };
    httpd_uri_t u_rst    = { .uri="/api/restart",.method=HTTP_POST, .handler=restart_handler };

    httpd_register_uri_handler(s_server, &u_page);
    httpd_register_uri_handler(s_server, &u_get);
    httpd_register_uri_handler(s_server, &u_post);
    httpd_register_uri_handler(s_server, &u_rst);

    touch();
}
