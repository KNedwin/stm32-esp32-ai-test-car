#!/bin/bash
# ESP32 版主机单元测试（编译前必跑，无硬件依赖）
set -e
cd "$(dirname "$0")"
mkdir -p build
MAIN=../esp32_tts_rtos/main

echo "========================================"
echo "[1/2] rfid_logic 触发/去重逻辑单元测试（与 STM32 版相同代码）"
gcc -std=c11 -Wall -Wextra -Werror \
    -I "$MAIN" \
    test_logic.c "$MAIN/rfid_logic.c" \
    -o build/test_logic
./build/test_logic

echo "========================================"
echo "[2/2] card_parse 帧解析单元测试"
gcc -std=c11 -Wall -Wextra -Werror \
    -I "$MAIN" \
    test_card_parse.c "$MAIN/card_parse.c" \
    -o build/test_card_parse
./build/test_card_parse

echo "========================================"
echo "全部测试完成"
