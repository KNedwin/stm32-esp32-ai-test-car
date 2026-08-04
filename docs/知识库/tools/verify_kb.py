#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""知识库验证脚本（本项目专用，兼容 v13 流程）
检查：文档完整性 / 章节与表格最低标准 / 参数-源码一致性抽查
"""
import os, sys, re

KB = "<HOME>/新能源小车/docs/知识库"
PROJECTS = ["stm32_rfid_tts_rtos", "stm32_rfid_tts_baremetal",
            "esp32_tts_rtos", "esp32_tts_baremetal"]

EXPECTED = ["00_阅读指南.md","01_项目介绍.md","02_硬件配置.md","03_系统架构.md",
            "04_功能模块.md","05_通信协议.md","06_关键参数表.md","07_已知问题与建议.md",
            "08_数据流与控制流.md","09_项目结构总览.md","10_代码语义化.md","11_常见问题清单.md"]

def check(name):
    d = f"{KB}/{name}"
    fails = []
    # 1. 文档完整性
    for f in EXPECTED:
        if not os.path.isfile(f"{d}/{f}"):
            fails.append(f"缺少 {f}")
    # 2. 章节检查（每文档至少 3 个 ##）
    for f in EXPECTED:
        p = f"{d}/{f}"
        if not os.path.isfile(p):
            continue
        txt = open(p, encoding="utf-8").read()
        h2 = len(re.findall(r"^## ", txt, re.M))
        if h2 < 3:
            fails.append(f"{f} 章节过少({h2})")
    # 3. 表格最低标准
    p06 = f"{d}/06_关键参数表.md"
    if os.path.isfile(p06):
        rows = len(re.findall(r"^\| [A-Z_0-9/]+ \|", open(p06, encoding="utf-8").read(), re.M))
        if rows < 20:
            fails.append(f"06_关键参数表 表格行数不足({rows}<20)")
    p11 = f"{d}/11_常见问题清单.md"
    if os.path.isfile(p11):
        q = len(re.findall(r"^### Q\d+", open(p11, encoding="utf-8").read(), re.M))
        if q != 20:
            fails.append(f"11_常见问题清单 Q 数量({q}≠20)")
    p02 = f"{d}/02_硬件配置.md"
    if os.path.isfile(p02):
        t = open(p02, encoding="utf-8").read()
        pins = len(re.findall(r"^\| P[A-D]\d+", t, re.M)) + len(re.findall(r"^\| GPIO\d+", t, re.M))
        if pins < 10:
            fails.append(f"02_硬件配置 引脚表行数({pins})")
    p04 = f"{d}/04_功能模块.md"
    if os.path.isfile(p04):
        mods = len(re.findall(r"^## \d+\.", open(p04, encoding="utf-8").read(), re.M))
        if mods < 8:
            fails.append(f"04_功能模块 模块数({mods}<8)")
    # 4. 参数-源码一致性抽查（v0.5 事实）
    snap = f"{d}/_v10_snapshot"
    checks = [
        ("MOTOR_START_LATE_TIME_MS", r"2000"),
        ("TRIGGER_STOP_RAMP_TIME_S", r"2"),
        ("TRIGGER_WAIT_TIME_S", r"10"),
        ("SPEAK_DEDUP_TIME_S", r"10"),
    ]
    if name == "stm32_rfid_tts_rtos":   # heap 仅 RTOS 版存在
        checks.append(("configTOTAL_HEAP_SIZE", r"8192"))
    for cfg, pat in checks:
        found = False
        for dirpath, _, files in os.walk(snap):
            for fn in files:
                if fn == "config.h" or (fn == "FreeRTOSConfig.h" and cfg == "configTOTAL_HEAP_SIZE"):
                    txt = open(os.path.join(dirpath, fn), encoding="utf-8", errors="ignore").read()
                    if re.search(rf"{cfg}.*{pat}", txt):
                        found = True
        if not found:
            fails.append(f"参数 {cfg}={pat} 未在快照 config.h 中找到")
    return fails

all_ok = True
for p in PROJECTS:
    fails = check(p)
    status = "✅ 通过" if not fails else f"❌ {len(fails)} 项失败"
    print(f"[{p}] {status}")
    for f in fails:
        print(f"   - {f}")
        all_ok = False
print("\n验证结果:", "全部通过" if all_ok else "存在失败项")
sys.exit(0 if all_ok else 1)
