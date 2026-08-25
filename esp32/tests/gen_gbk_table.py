#!/usr/bin/env python3
"""生成 GB2312→Unicode 映射表头 gbk_utf8_table.h

覆盖区：GBK 双字节区 high 0xA1~0xF7, low 0xA1~0xFE（符号+一级+二级汉字，87×94=8178 格）
重新生成：python3 gen_gbk_table.py > ../components/common/gbk_utf8_table.h
"""
import sys

ROWS = range(0xA1, 0xF8)   # 87 行
COLS = range(0xA1, 0xFF)   # 94 列

print("/* 自动生成：GB2312→Unicode 映射表（勿手改）")
print(" * 覆盖：high 0xA1~0xF7 × low 0xA1~0xFE，0 表示无映射")
print(" * 重新生成：cd esp32/tests && python3 gen_gbk_table.py > ../components/common/gbk_utf8_table.h */")
print("#ifndef __GBK_UTF8_TABLE_H")
print("#define __GBK_UTF8_TABLE_H")
print("")
print("static const uint16_t s_gbk2uni[%d] = {" % (len(list(ROWS)) * len(list(COLS)),))

total = 0
for high in range(0xA1, 0xF8):
    print("    /* row 0x%02X */" % high)
    line = []
    for low in range(0xA1, 0xFF):
        try:
            uni = ord(bytes([high, low]).decode("gbk"))
            line.append("0x%04X" % uni)
        except UnicodeDecodeError:
            line.append("0x0000")
        total += 1
    for k in range(0, len(line), 10):
        print("    " + ", ".join(line[k:k+10]) + ",")

print("};")
print("")
print("#endif /* __GBK_UTF8_TABLE_H */")
sys.stderr.write("entries: %d\n" % total)
