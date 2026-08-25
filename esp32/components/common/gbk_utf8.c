#include "gbk_utf8.h"
#include "gbk_utf8_table.h"
#include <string.h>

/* ============ 内部：Unicode → UTF-8（返回字节数 1~3） ============ */
static int uni_to_utf8(uint16_t uni, char *out)
{
    if (uni < 0x80)
    {
        out[0] = (char)uni;
        return 1;
    }
    if (uni < 0x800)
    {
        out[0] = (char)(0xC0 | (uni >> 6));
        out[1] = (char)(0x80 | (uni & 0x3F));
        return 2;
    }
    out[0] = (char)(0xE0 | (uni >> 12));
    out[1] = (char)(0x80 | ((uni >> 6) & 0x3F));
    out[2] = (char)(0x80 | (uni & 0x3F));
    return 3;
}

/* ============ 内部：UTF-8 单字符 → Unicode（返回消耗字节数，失败 0） ============ */
static int utf8_to_uni(const char *u8, size_t remain, uint16_t *uni)
{
    uint8_t c = (uint8_t)u8[0];
    if (c < 0x80)
    {
        *uni = c;
        return 1;
    }
    if ((c & 0xE0) == 0xC0 && remain >= 2)
    {
        *uni = (uint16_t)(((c & 0x1F) << 6) | (u8[1] & 0x3F));
        return 2;
    }
    if ((c & 0xF0) == 0xE0 && remain >= 3)
    {
        *uni = (uint16_t)(((c & 0x0F) << 12) | ((u8[1] & 0x3F) << 6) | (u8[2] & 0x3F));
        return 3;
    }
    *uni = 0;
    return 0;   /* 无效序列 */
}

/* ============ 内部：Unicode → GBK 双字节（线性查表，0=无映射） ============ */
static uint16_t uni_to_gbk(uint16_t uni)
{
    uint32_t k;
    for (k = 0; k < sizeof(s_gbk2uni) / sizeof(s_gbk2uni[0]); k++)
    {
        if (s_gbk2uni[k] == uni)
            return (uint16_t)(0x8000u | (uint16_t)k);   /* 置标志位防 0 歧义 */
    }
    return 0;
}

/* ============ 公共 API ============ */
int gbk_to_utf8(const uint8_t *gbk, size_t gbk_len, char *out, size_t out_size)
{
    size_t i = 0, o = 0;
    char tmp[4];
    int n;

    if (!out || out_size == 0) return -1;

    while (i < gbk_len)
    {
        uint8_t b = gbk[i];
        if (b < 0x80)
        {
            tmp[0] = (char)b;
            n = 1;
            i++;
        }
        else if (i + 1 < gbk_len &&
                 b >= 0xA1 && b <= 0xF7 &&
                 gbk[i + 1] >= 0xA1 && gbk[i + 1] <= 0xFE)
        {
            uint16_t uni = s_gbk2uni[(b - 0xA1) * 94 + (gbk[i + 1] - 0xA1)];
            i += 2;
            if (uni == 0) { tmp[0] = '?'; n = 1; }
            else          { n = uni_to_utf8(uni, tmp); }
        }
        else
        {
            tmp[0] = '?';
            n = 1;
            i++;
        }

        if (o + (size_t)n + 1 > out_size) return -1;
        memcpy(out + o, tmp, (size_t)n);
        o += (size_t)n;
    }
    out[o] = '\0';
    return (int)o;
}

int utf8_to_gbk(const char *utf8, size_t utf8_len, uint8_t *out, size_t out_size)
{
    size_t i = 0, o = 0;
    uint8_t tmp[2];
    int n;

    if (!out || out_size == 0) return -1;

    while (i < utf8_len)
    {
        uint16_t uni = 0;
        int used = utf8_to_uni(utf8 + i, utf8_len - i, &uni);
        if (used == 0) { tmp[0] = '?'; n = 1; i++; }
        else
        {
            i += (size_t)used;
            if (uni < 0x80) { tmp[0] = (uint8_t)uni; n = 1; }
            else
            {
                uint16_t g = uni_to_gbk(uni);
                if (g == 0) { tmp[0] = '?'; n = 1; }
                else
                {
                    uint16_t idx = g & 0x7FFF;
                    tmp[0] = (uint8_t)(0xA1 + idx / 94);
                    tmp[1] = (uint8_t)(0xA1 + idx % 94);
                    n = 2;
                }
            }
        }

        if (o + (size_t)n > out_size) return -1;
        memcpy(out + o, tmp, (size_t)n);
        o += (size_t)n;
    }
    return (int)o;
}
