#!/bin/bash
# ESP32 版主机单元测试（编译前必跑，无硬件依赖）
# 共享组件位于 components/common（两版项目共用，测试指向 RTOS 版同一份）
set -e
cd "$(dirname "$0")"
mkdir -p build
COMMON=../components/common

echo "========================================"
echo "[1/3] rfid_logic 触发/去重逻辑单元测试（与 STM32 版相同代码）"
gcc -std=c11 -Wall -Wextra -Werror \
    -I "$COMMON" \
    test_logic.c "$COMMON/rfid_logic.c" \
    -o build/test_logic
./build/test_logic

echo "========================================"
echo "[2/3] card_parse 帧解析单元测试"
gcc -std=c11 -Wall -Wextra -Werror \
    -I "$COMMON" \
    test_card_parse.c "$COMMON/card_parse.c" \
    -o build/test_card_parse
./build/test_card_parse

echo "========================================"
echo "[3/4] motor_logic 电机状态机单元测试（真实 C 代码）"
gcc -std=c11 -Wall -Wextra -Werror \
    -I "$COMMON" \
    test_motor_logic.c "$COMMON/motor_logic.c" \
    -o build/test_motor_logic
./build/test_motor_logic

echo "========================================"
echo "[4/4] gbk_utf8 编码转换单元测试（GB2312⇄UTF-8）"
gcc -std=c11 -Wall -Wextra -Werror \
    -I "$COMMON" \
    test_gbk_utf8.c "$COMMON/gbk_utf8.c" \
    -o build/test_gbk_utf8
./build/test_gbk_utf8

echo "========================================"
echo "共享组件漂移检查（两版 main 差异文件清单）"
diff -rq ../esp32_tts_rtos/main ../esp32_tts_baremetal/main \
    -x app_main.c -x rfid_task.c -x rfid_task.h -x motor_task.c -x motor_task.h \
    -x rfid_process.c -x rfid_process.h -x motor_process.c -x motor_process.h \
    -x CMakeLists.txt || { echo "ERROR: 两版 main 存在意外差异（共享文件漂移）"; exit 1; }
echo "OK：两版 main 差异仅限预期的 4 组文件"

echo "========================================"
echo "全部测试完成"
