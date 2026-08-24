#include "ws2812.h"
#include "config.h"
#include "driver/rmt_tx.h"
#include "driver/rmt_encoder.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

/* ============ WS2812 协议参数（10MHz 时基） ============ */
#define RES_HZ 10000000

/* ============ 颜色表（RGB 顺序） ============ */
typedef struct { uint8_t r, g, b; } rgb_t;
static const rgb_t color_table[LED_COLOR_COUNT] = {
    [LED_COLOR_BOOT]    = {  0,  0, 60},
    [LED_COLOR_IDLE]    = { 10, 10, 10},
    [LED_COLOR_CARD]    = {  0, 60,  0},
    [LED_COLOR_SLOWING] = { 60, 40,  0},
    [LED_COLOR_STOPPED] = { 60,  0,  0},
    [LED_COLOR_RAMPUP]  = { 40, 20,  0},
    [LED_COLOR_OFF]     = {  0,  0,  0},
};

/* ============ Simple encoder（参考 ESP-IDF led_strip_simple_encoder 示例） ============ */
static const rmt_symbol_word_t ws_zero = {
    .level0 = 1, .duration0 = 0.3 * RES_HZ / 1000000,
    .level1 = 0, .duration1 = 0.9 * RES_HZ / 1000000,
};
static const rmt_symbol_word_t ws_one = {
    .level0 = 1, .duration0 = 0.9 * RES_HZ / 1000000,
    .level1 = 0, .duration1 = 0.3 * RES_HZ / 1000000,
};
static const rmt_symbol_word_t ws_reset = {
    .level0 = 0, .duration0 = RES_HZ / 1000000 * 50 / 2,
    .level1 = 0, .duration1 = RES_HZ / 1000000 * 50 / 2,
};

static size_t ws_encode_callback(const void *data, size_t data_size,
                                  size_t symbols_written, size_t symbols_free,
                                  rmt_symbol_word_t *symbols, bool *done, void *arg)
{
    if (symbols_free < 8) return 0;
    size_t pos = symbols_written / 8;
    uint8_t *bytes = (uint8_t *)data;
    if (pos < data_size) {
        size_t si = 0;
        for (int mask = 0x80; mask != 0; mask >>= 1)
            symbols[si++] = (bytes[pos] & mask) ? ws_one : ws_zero;
        return si;
    } else {
        symbols[0] = ws_reset;
        *done = 1;
        return 1;
    }
}

/* ============ 公共 API ============ */
static rmt_channel_handle_t s_ch = NULL;
static rmt_encoder_handle_t  s_enc = NULL;

void WS2812_Init(void)
{
    rmt_tx_channel_config_t cfg = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .gpio_num = LED_WS2812_PIN,
        .resolution_hz = RES_HZ,
        .mem_block_symbols = 64,
        .trans_queue_depth = 4,
    };
    rmt_new_tx_channel(&cfg, &s_ch);

    rmt_simple_encoder_config_t ecfg = {.callback = ws_encode_callback};
    rmt_new_simple_encoder(&ecfg, &s_enc);

    rmt_enable(s_ch);

    WS2812_SetColor(LED_COLOR_BOOT);
}

void WS2812_SetColor(led_color_t color)
{
    if (!s_ch || !s_enc) return;
    if (color >= LED_COLOR_COUNT) return;

    const rgb_t *c = &color_table[color];
    uint8_t grb[3] = {c->g, c->r, c->b};  /* WS2812B GRB 顺序 */
    rmt_transmit_config_t tc = {.loop_count = 0};
    rmt_transmit(s_ch, s_enc, grb, sizeof(grb), &tc);
    rmt_tx_wait_all_done(s_ch, 200);
}
