#!/bin/bash
# 编译并运行主机单元测试 + 电机仿真测试（Linux，无第三方依赖）
set -e
cd "$(dirname "$0")"
mkdir -p build
ROOT=../rfid_tts_rtos

echo "========================================"
echo "[1/3] Card.c 协议解析单元测试"
gcc -std=c11 -Wall -Wextra -Werror \
    -I hal_stub -I "$ROOT/hardware/rfid_card" \
    test_card.c "$ROOT/hardware/rfid_card/Card.c" \
    -o build/test_card
./build/test_card

echo "========================================"
echo "[2/3] rfid_logic 触发/去重逻辑单元测试"
gcc -std=c11 -Wall -Wextra -Werror \
    -I "$ROOT" -I "$ROOT/Task" \
    test_logic.c "$ROOT/Task/rfid_logic.c" \
    -o build/test_logic
./build/test_logic

echo "========================================"
echo "[3/4] motor_logic 电机状态机单元测试（真实 C 代码）"
gcc -std=c11 -Wall -Wextra -Werror \
    -I "$ROOT" -I "$ROOT/Task" \
    test_motor_logic.c "$ROOT/Task/motor_logic.c" \
    -o build/test_motor_logic
./build/test_motor_logic

echo "========================================"
echo "[4/4] 电机状态机仿真测试（Python，规格级）"
python3 sim_motor.py

echo "========================================"
echo "全部测试完成"
