#include "param_cli.h"
#include "nvs_params.h"
#include "Debug.h"          /* 响应走 USART3 调试口；printf 会进 TTS 语音口! */
#include "stm32f1xx_hal.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* 依赖注入的动作钩子：由 isp_jump / 系统层提供实现 */
__attribute__((weak)) void param_cli_do_isp(void)    { }
__attribute__((weak)) void param_cli_do_reboot(void) { }

#define LINE_MAX 192

static char    line_buf[LINE_MAX];
static uint8_t line_len = 0;

static uint8_t s_isp_requested = 0;

uint8_t ParamCli_ShouldEnterIsp(void) { return s_isp_requested; }

void ParamCli_Init(void)
{
    /* 使能 USART3 接收（CubeMX 只配了发送） */
    USART3->CR1 |= USART_CR1_RE;
}

/* ---------- SET 标量关键字表 ---------- */
static uint8_t set_scalar(const char *key, double v)
{
    if      (!strcmp(key, "late_s"))  g_params.late_ms = (uint32_t)(v * 1000);
    else if (!strcmp(key, "slow_s"))  g_params.slow_ms = (uint32_t)(v * 1000);
    else if (!strcmp(key, "target"))  g_params.target_speed = (uint16_t)v;
    else if (!strcmp(key, "dir"))     g_params.motor_dir = (v != 0);
    else if (!strcmp(key, "sr_s"))    g_params.stop_ramp_ms = (uint32_t)(v * 1000);
    else if (!strcmp(key, "ws_s"))    g_params.wait_ms = (uint32_t)(v * 1000);
    else if (!strcmp(key, "led_s"))   g_params.led_on_ms = (uint32_t)(v * 1000);
    else if (!strcmp(key, "dedup_s")) g_params.dedup_ms = (uint32_t)(v * 1000);
    else if (!strcmp(key, "poll_s"))  g_params.rfid_poll_ms = (uint32_t)(v * 1000);
    else if (!strcmp(key, "ci_s"))    g_params.count_interval_ms = (uint32_t)(v * 1000);
    else if (!strcmp(key, "as_s"))    g_params.autostop_ms = (uint32_t)(v * 1000);
    else return 0;
    return 1;
}

static void respond(const char *s)
{
    Dbg_Printf("> %s\r\n", s);
}

static void hex_to_bytes(const char *hex, uint8_t *out, uint8_t max)
{
    uint8_t n = 0;
    while (hex[0] && hex[1] && n < max)
    {
        uint8_t hi = hex[0], lo = hex[1];
        out[n++] = (uint8_t)((hi <= '9' ? hi - '0' : (hi | 32) - 'a' + 10) << 4)
                 | (uint8_t)((lo <= '9' ? lo - '0' : (lo | 32) - 'a' + 10));
        hex += 2;
    }
}

/* 执行一行命令（纯逻辑，可主机测试；硬件动作经钩子） */
void param_cli_execute(char *line)
{
    char *save = NULL;
    const char *sep = " \t";
    char *cmd = strtok_r(line, sep, &save);
    if (!cmd) return;

    if (!strcmp(cmd, "HELP"))
    {
        /* Dbg_Printf 单条限 128 字节，HELP 拆两行 */
        respond("cmds: GET all|<key> ; SET <key> <val> ; SAVE ; DUMP ; ISP ; REBOOT");
        respond("      SET win_add <s> <d> <p> / win_del <i> ; SET rule_add <gbkhex> <cnt> <spk> / rule_del <i>");
    }
    else if (!strcmp(cmd, "GET"))
    {
        char *k = strtok_r(NULL, sep, &save);
        if (!k || !strcmp(k, "all"))
        {
            Dbg_Printf("> {\"late_s\":%lu,\"slow_s\":%lu,\"target\":%u,\"dir\":%u,"
                   "\"sr_s\":%lu,\"ws_s\":%lu,\"led_s\":%lu,\"dedup_s\":%lu,"
                   "\"poll_s\":%lu,\"ci_s\":%lu,\"as_s\":%lu,"
                   "\"win_n\":%u,\"rule_n\":%u}\r\n",
                   (unsigned long)(g_params.late_ms/1000), (unsigned long)(g_params.slow_ms/1000),
                   g_params.target_speed, g_params.motor_dir,
                   (unsigned long)(g_params.stop_ramp_ms/1000), (unsigned long)(g_params.wait_ms/1000),
                   (unsigned long)(g_params.led_on_ms/1000), (unsigned long)(g_params.dedup_ms/1000),
                   (unsigned long)(g_params.rfid_poll_ms/1000), (unsigned long)(g_params.count_interval_ms/1000),
                   (unsigned long)(g_params.autostop_ms/1000),
                   g_params.slowwin_count, g_params.rule_count);
        }
        else respond("ERR:key");
    }
    else if (!strcmp(cmd, "SET"))
    {
        char *k = strtok_r(NULL, sep, &save);
        char *v = strtok_r(NULL, sep, &save);
        if (!k || !v) { respond("ERR:args"); return; }

        if (!strncmp(k, "win_", 4))
        {
            if (!strcmp(k, "win_add") && v)
            {
                char *d = strtok_r(NULL, sep, &save);
                char *p = d ? strtok_r(NULL, sep, &save) : NULL;
                if (!d || !p) { respond("ERR:args"); return; }
                if (g_params.slowwin_count >= 8) { respond("ERR:full"); return; }
                g_params.slowwins[g_params.slowwin_count].start_ms = (uint32_t)(atof(v) * 1000);
                g_params.slowwins[g_params.slowwin_count].dur_ms   = (uint32_t)(atof(d) * 1000);
                g_params.slowwins[g_params.slowwin_count].pct      = (uint8_t)atoi(p);
                g_params.slowwin_count++;
                params_apply();     /* 即时生效 */
                respond("OK");
            }
            else if (!strcmp(k, "win_del") && v)
            {
                int idx = atoi(v);
                if (idx < 0 || idx >= g_params.slowwin_count) { respond("ERR:range"); return; }
                for (int i = idx; i + 1 < g_params.slowwin_count; i++)
                    g_params.slowwins[i] = g_params.slowwins[i + 1];
                g_params.slowwin_count--;
                params_apply();     /* 即时生效 */
                respond("OK");
            }
            return;
        }
        if (!strncmp(k, "rule_", 5))
        {
            if (!strcmp(k, "rule_add"))
            {
                char *hx = v;
                char *c  = strtok_r(NULL, sep, &save);
                char *s  = c ? strtok_r(NULL, sep, &save) : NULL;
                if (!c || !s) { respond("ERR:args"); return; }
                if (g_params.rule_count >= 8) { respond("ERR:full"); return; }
                memset(g_params.rules[g_params.rule_count].word, 0, 16);
                hex_to_bytes(hx, g_params.rules[g_params.rule_count].word, 16);
                g_params.rules[g_params.rule_count].len       = (uint8_t)(strlen(hx) / 2);
                g_params.rules[g_params.rule_count].count_req = (uint8_t)atoi(c);
                g_params.rules[g_params.rule_count].speak_en  = (uint8_t)atoi(s);
                g_params.rule_count++;
                params_apply();     /* 即时生效 */
                respond("OK");
            }
            else if (!strcmp(k, "rule_del"))
            {
                int idx = atoi(v);
                if (idx < 0 || idx >= g_params.rule_count) { respond("ERR:range"); return; }
                for (int i = idx; i + 1 < g_params.rule_count; i++)
                    g_params.rules[i] = g_params.rules[i + 1];
                g_params.rule_count--;
                params_apply();     /* 即时生效 */
                respond("OK");
            }
            return;
        }
        if (!set_scalar(k, atof(v))) { respond("ERR:key"); return; }
        params_apply();             /* 即时生效 */
        respond("OK");
    }
    else if (!strcmp(cmd, "SAVE"))
    {
        if (params_save() == 0) respond("OK:saved");
        else                    respond("ERR:save");
        params_apply();
    }
    else if (!strcmp(cmd, "DUMP"))
    {
        const uint8_t *p = (const uint8_t *)0x0800FC00UL;
        Dbg_Printf("> DUMP:");
        for (int i = 0; i < 64; i++) Dbg_Printf(" %02X", p[i]);
        Dbg_Printf("\r\n");
    }
    else if (!strcmp(cmd, "ISP"))
    {
        respond("OK:entering ISP");
        s_isp_requested = 1;
        param_cli_do_isp();
    }
    else if (!strcmp(cmd, "REBOOT"))
    {
        respond("OK:reboot");
        param_cli_do_reboot();
    }
    else respond("ERR:unknown");
}

void ParamCli_Poll(void)
{
    static uint8_t rx_ready = 0;

    if (!rx_ready)
    {
        USART3->CR1 |= USART_CR1_RE;
        rx_ready = 1;
        return;
    }

    while ((USART3->SR & USART_SR_RXNE) != 0)
    {
        char ch = (char)(USART3->DR & 0xFF);
        if (ch == '\r') continue;
        if (ch == '\n' || ch == '\b')
        {
            if (ch == '\b' && line_len > 0) { line_len--; continue; }
            line_buf[line_len] = '\0';
            param_cli_execute(line_buf);
            line_len = 0;
            continue;
        }
        if (line_len < LINE_MAX - 1) line_buf[line_len++] = ch;
    }
}
