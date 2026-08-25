/* gbk_utf8 编码转换单元测试（纯逻辑，无硬件依赖） */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "gbk_utf8.h"

static int pass = 0, fail = 0;

#define CHECK(cond, name)                                                    \
    do {                                                                     \
        if (cond) { pass++; }                                                \
        else        { fail++; printf("FAIL: %s\n", name); }                  \
    } while (0)

int main(void)
{
    char    u8[64];
    int     n;
    uint8_t back[64];
    int     m;

    /* 1. 太阳：GBK → UTF-8 */
    {
        uint8_t taiyang_gbk[] = {0xCC, 0xAB, 0xD1, 0xF4};
        n = gbk_to_utf8(taiyang_gbk, 4, u8, sizeof(u8));
        CHECK(n == 6 && (uint8_t)u8[0] == 0xE5 && (uint8_t)u8[1] == 0xA4 &&
              (uint8_t)u8[2] == 0xAA && (uint8_t)u8[3] == 0xE9,
              "太阳 GBK→UTF8");
    }

    /* 2. 太阳：UTF-8 → GBK 回程 */
    {
        const char *src = "\xe5\xa4\xaa\xe9\x98\xb3";
        m = utf8_to_gbk(src, 6, back, sizeof(back));
        CHECK(m == 4 && back[0] == 0xCC && back[1] == 0xAB &&
              back[2] == 0xD1 && back[3] == 0xF4, "太阳 UTF8→GBK 回程");
    }

    /* 3. 地球往返 */
    {
        uint8_t diqiu[] = {0xB5, 0xD8, 0xC7, 0xF2};
        n = gbk_to_utf8(diqiu, 4, u8, sizeof(u8));
        CHECK(n == 6, "地球 GBK→UTF8");
        m = utf8_to_gbk(u8, (size_t)n, back, sizeof(back));
        CHECK(m == 4 && memcmp(back, diqiu, 4) == 0, "地球 UTF8→GBK 回程");
    }

    /* 4. ASCII 混合："AB12" 直通 */
    n = gbk_to_utf8((const uint8_t *)"AB12", 4, u8, sizeof(u8));
    CHECK(n == 4 && memcmp(u8, "AB12", 4) == 0, "ASCII 直通");

    /* 5. 空串 */
    n = gbk_to_utf8((const uint8_t *)"", 0, u8, sizeof(u8));
    CHECK(n == 0 && u8[0] == '\0', "空串");

    /* 6. 非法字节降级 '?'（单高位字节无跟随） */
    {
        uint8_t bad[] = {0xFF};
        n = gbk_to_utf8(bad, 1, u8, sizeof(u8));
        CHECK(n == 1 && u8[0] == '?', "非法字节降级");
    }

    /* 7. 缓冲区不足返回负数 */
    {
        uint8_t taiyang[] = {0xCC, 0xAB, 0xD1, 0xF4};
        n = gbk_to_utf8(taiyang, 4, u8, 3);
        CHECK(n < 0, "缓冲区不足防护");
    }

    /* 8. 混合串 "A太B"：GBK A(41) CC AB D1 F4 B(42) → UTF-8 */
    {
        uint8_t mix[] = {'A', 0xCC, 0xAB, 0xD1, 0xF4, 'B'};
        n = gbk_to_utf8(mix, 6, u8, sizeof(u8));
        CHECK(n == 8 && u8[0] == 'A' && (uint8_t)u8[1] == 0xE5 && u8[7] == 'B',
              "ASCII+中文混合");
    }

    /* 9. 多字词组 "新能源小车" 往返 */
    {
        const char *src = "\xe6\x96\xb0\xe8\x83\xbd\xe6\xba\x90"
                          "\xe5\xb0\x8f\xe8\xbd\xa6";
        m = utf8_to_gbk(src, 15, back, sizeof(back));
        CHECK(m == 10, "新能源小车 UTF8→GBK");
        n = gbk_to_utf8(back, (size_t)m, u8, sizeof(u8));
        CHECK(n == 15 && memcmp(u8, src, 15) == 0, "新能源小车 往返一致");
    }

    printf("=== gbk_utf8 单元测试 ===\n");
    printf("通过 %d，失败 %d\n", pass, fail);
    return fail != 0;
}
