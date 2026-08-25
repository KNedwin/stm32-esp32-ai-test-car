#ifndef __GBK_UTF8_H
#define __GBK_UTF8_H

#include <stdint.h>
#include <stddef.h>

/* GBK(GB2312 区) ⇄ UTF-8 转换
 * 覆盖：ASCII 直通 + GB2312 符号/一级/二级汉字（8178 项映射表，Flash 常量）
 * 表外字符降级为 '?'；缓冲区不足返回负数。 */

/* GBK → UTF-8。返回写入 out 的字节数（不含结尾 '\0'），负数=缓冲区不足 */
int gbk_to_utf8(const uint8_t *gbk, size_t gbk_len, char *out, size_t out_size);

/* UTF-8 → GBK。返回写入 out 的字节数，负数=缓冲区不足 */
int utf8_to_gbk(const char *utf8, size_t utf8_len, uint8_t *out, size_t out_size);

#endif /* __GBK_UTF8_H */
