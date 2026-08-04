#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""MCU 知识库第 1 层提取（Linux 等效 bootstrap.cmd）
对 4 个复刻项目生成 project_context.json + _v10_snapshot/ + call_graph_hint.json
用法: python3 extract.py
"""
import json, os, re, shutil, sys, datetime

ROOT = "<HOME>/新能源小车"
KB = f"{ROOT}/docs/知识库"

PROJECTS = [
    {
        "name": "stm32_rfid_tts_rtos",
        "root": f"{ROOT}/stm32/rfid_tts_rtos",
        "build_system": "cmake_cubemx",
        "device": "STM32F103C8T6",
        "flash": {"start": "0x08000000", "size": "0x10000"},
        "ram":    {"start": "0x20000000", "size": "0x5000"},
        "defines": ["USE_HAL_DRIVER", "STM32F103xB", "DEBUG"],
        "src_globs": [
            "Core/Src/*.c",
            "hardware/**/*.c",
            "Task/*.c",
        ],
        "inc_dirs": [
            "Core/Inc",
            "Drivers/STM32F1xx_HAL_Driver/Inc",
            "Drivers/STM32F1xx_HAL_Driver/Inc/Legacy",
            "Drivers/CMSIS/Device/ST/STM32F1xx/Include",
            "Drivers/CMSIS/Include",
            "Middlewares/Third_Party/FreeRTOS/Source/include",
            "Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS",
            "Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM3",
            ".",
            "hardware/USART", "hardware/rfid_card", "hardware/pwm",
            "hardware/LED", "hardware/ADC", "hardware/DEBUG", "Task",
        ],
    },
    {
        "name": "stm32_rfid_tts_baremetal",
        "root": f"{ROOT}/stm32/rfid_tts_baremetal",
        "build_system": "cmake_cubemx",
        "device": "STM32F103C8T6",
        "flash": {"start": "0x08000000", "size": "0x10000"},
        "ram":    {"start": "0x20000000", "size": "0x5000"},
        "defines": ["USE_HAL_DRIVER", "STM32F103xB", "DEBUG"],
        "src_globs": [
            "Core/Src/*.c",
            "hardware/**/*.c",
            "Task/*.c",
        ],
        "inc_dirs": [
            "Core/Inc",
            "Drivers/STM32F1xx_HAL_Driver/Inc",
            "Drivers/STM32F1xx_HAL_Driver/Inc/Legacy",
            "Drivers/CMSIS/Device/ST/STM32F1xx/Include",
            "Drivers/CMSIS/Include",
            ".",
            "hardware/USART", "hardware/rfid_card", "hardware/pwm",
            "hardware/LED", "hardware/ADC", "hardware/DEBUG", "Task",
        ],
    },
    {
        "name": "esp32_tts_rtos",
        "root": f"{ROOT}/esp32/esp32_tts_rtos",
        "build_system": "esp_idf",
        "device": "ESP32-S3",
        "flash": {"start": "0x00000000", "size": "0x200000"},
        "ram":    {"start": "0x3FC88000", "size": "0x80000"},
        "defines": ["ESP_PLATFORM", "IDF_VER=\"v6.0.2\""],
        "src_globs": [
            "main/*.c",
        ],
        "extra_dirs": [f"{ROOT}/esp32/components/common"],
        "inc_dirs": [
            "main",
        ],
        "extra_inc_dirs": [f"{ROOT}/esp32/components/common"],
    },
    {
        "name": "esp32_tts_baremetal",
        "root": f"{ROOT}/esp32/esp32_tts_baremetal",
        "build_system": "esp_idf",
        "device": "ESP32-S3",
        "flash": {"start": "0x00000000", "size": "0x200000"},
        "ram":    {"start": "0x3FC88000", "size": "0x80000"},
        "defines": ["ESP_PLATFORM", "IDF_VER=\"v6.0.2\""],
        "src_globs": [
            "main/*.c",
        ],
        "extra_dirs": [f"{ROOT}/esp32/components/common"],
        "inc_dirs": [
            "main",
        ],
        "extra_inc_dirs": [f"{ROOT}/esp32/components/common"],
    },
]

def expand_globs(proj):
    files = []
    for g in proj.get("extra_dirs", []):
        for dirpath, dirnames, filenames in os.walk(g):
            rel = os.path.relpath(dirpath, g)
            for fn in filenames:
                if fn.endswith(".c"):
                    files.append(f"<extra>/{fn}")
    for g in proj["src_globs"]:
        base = g.split("*")[0]
        for dirpath, dirnames, filenames in os.walk(proj["root"]):
            # 跳过 build、docs、.git
            rel = os.path.relpath(dirpath, proj["root"])
            if any(part in ("build", "docs", ".git", "_v10_snapshot") for part in rel.split(os.sep)):
                continue
            for fn in filenames:
                if not fn.endswith(".c"):
                    continue
                full = os.path.join(dirpath, fn)
                relpath = os.path.relpath(full, proj["root"])
                import fnmatch
                for pat in proj["src_globs"]:
                    if fnmatch.fnmatch(relpath, pat):
                        files.append(relpath)
                        break
    # 去重保序
    seen = set()
    out = []
    for f in files:
        if f not in seen:
            seen.add(f)
            out.append(f)
    return out

def classify(relpath):
    if relpath.startswith("Drivers/") or "Middlewares/" in relpath:
        return "third_party"
    if "stm32f1xx_hal" in relpath or relpath.startswith("Core/Src/system") or relpath.startswith("Core/Src/syscalls") or relpath.startswith("Core/Src/sysmem"):
        return "hal"
    if "components/common" in relpath and ("driver" not in relpath):
        return "business"
    if relpath.startswith("main/") or relpath.startswith("Task/") or relpath.startswith("hardware/") or "components/common" in relpath:
        return "business"
    return "business"

def extract_funcs(filepath):
    """文本级函数定义粗提取"""
    funcs = []
    try:
        text = open(filepath, encoding="utf-8", errors="ignore").read()
    except Exception:
        return funcs
    # 匹配 "返回类型 函数名(参数) {"
    pattern = re.compile(r'^([A-Za-z_][\w\s\*]*?)\s+([A-Za-z_]\w*)\s*\(([^;{}]*)\)\s*\{', re.M)
    for m in pattern.finditer(text):
        ret, name = m.group(1).strip(), m.group(2)
        if name in ("if", "for", "while", "switch", "return", "sizeof"):
            continue
        if not re.match(r'^[A-Za-z_]\w*$', name):
            continue
        if " " in name or "*" in name:
            continue
        line = text.count("\n", 0, m.start()) + 1
        funcs.append({"name": name, "line": line, "signature": f"{ret} {name}({m.group(3).strip()})"})
    return funcs

def main():
    for proj in PROJECTS:
        name = proj["name"]
        outdir = f"{KB}/{name}"
        ctxdir = f"{outdir}/_v10_context"
        snap = f"{outdir}/_v10_snapshot"
        os.makedirs(f"{snap}/sources", exist_ok=True)
        os.makedirs(f"{snap}/headers", exist_ok=True)
        os.makedirs(ctxdir, exist_ok=True)

        srcs = expand_globs(proj)
        business = [s for s in srcs if classify(s) == "business"]
        third = [s for s in srcs if classify(s) == "third_party"]
        hal = [s for s in srcs if classify(s) == "hal"]

        # 复制快照
        copied = []
        for rel in srcs:
            if rel.startswith("<extra>/"):
                # 在快照中重建 components/common 目录
                src = os.path.join(proj.get("extra_dirs", [""])[0], os.path.basename(rel))
                rel2 = "components/common/" + os.path.basename(rel)
            else:
                src = os.path.join(proj["root"], rel)
                rel2 = rel
            dst = os.path.join(snap, "sources", rel2)
            os.makedirs(os.path.dirname(dst), exist_ok=True)
            try:
                shutil.copy2(src, dst)
                copied.append(rel2)
            except Exception as e:
                print(f"  skip {rel}: {e}")

        # 复制业务头文件
        headers = []
        for inc in proj["inc_dirs"] + proj.get("extra_inc_dirs", []):
            ipath = os.path.join(proj["root"], inc)
            if not os.path.isdir(ipath):
                continue
            for dirpath, _, files in os.walk(ipath):
                for fn in files:
                    if fn.endswith(".h"):
                        full = os.path.join(dirpath, fn)
                        rel = os.path.relpath(full, proj["root"])
                        dst = os.path.join(snap, "headers", rel)
                        os.makedirs(os.path.dirname(dst), exist_ok=True)
                        shutil.copy2(full, dst)
                        headers.append(rel)

        # 调用图
        callgraph = {"functions": []}
        for rel in business:
            fp = os.path.join(snap, "sources", rel)
            if not os.path.isfile(fp):
                continue
            for fn in extract_funcs(fp):
                fn["file"] = rel
                callgraph["functions"].append(fn)

        ctx = {
            "project_name": name,
            "project_root": proj["root"],
            "build_system": proj["build_system"],
            "target_device": proj["device"],
            "flash": proj["flash"],
            "ram": proj["ram"],
            "active_macros": proj["defines"],
            "include_paths": [os.path.join(proj["root"], i) for i in proj["inc_dirs"]],
            "business_sources": [{"relative_path": s, "classification": "business"} for s in business],
            "third_party_sources": third,
            "hal_sources": hal,
            "active_sources": [{"relative_path": s, "classification": classify(s)} for s in srcs],
            "header_files": headers,
            "extracted_at": datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
        }
        with open(f"{ctxdir}/project_context.json", "w", encoding="utf-8") as f:
            json.dump(ctx, f, ensure_ascii=False, indent=2)
        with open(f"{ctxdir}/call_graph_hint.json", "w", encoding="utf-8") as f:
            json.dump(callgraph, f, ensure_ascii=False, indent=1)

        print(f"[OK] {name}: 源文件 {len(srcs)}（业务 {len(business)}/HAL {len(hal)}/第三方 {len(third)}），头文件 {len(headers)}，函数 {len(callgraph['functions'])}")

if __name__ == "__main__":
    main()
