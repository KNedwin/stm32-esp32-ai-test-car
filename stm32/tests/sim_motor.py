#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""电机状态机行为仿真测试（规格级验证）
按 C 实现（motor_process.c）的算法复刻状态机，验证设计规格：
  晚启动A → 缓启动B → RUN → E~E+G降速窗口(绝对计时,一次) → 触发停车序列 →
  重启缓启动(不再经晚启动) → stop_time/1000s 绝对停止
运行：python3 sim_motor.py（无第三方依赖）
"""

# ---- 参数（与 config.h 一致）----
A = 2000          # 晚启动 ms
B = 4000          # 缓启动 ms
TARGET = 999
E = 42 * 1000     # 降速窗口起点 ms
G = 5 * 1000      # 降速窗口时长 ms
F = 50            # 降速百分比
H = 2 * 1000      # 停车减速 ms
I = 10 * 1000     # 静止等待 ms
MAX_RUN = 1000 * 1000  # 1000s 上限

IDLE, RAMPUP, RUN, SLOW, STOPPING, WAIT, STOP = range(7)

class Motor:
    def __init__(self, stop_time_ms):
        self.state = IDLE
        self.start_tick = 0          # 绝对计时起点
        self.state_tick = 0
        self.stop_time = stop_time_ms
        self.speed = 0
        self.ramp_start = 0
        self.triggered = False       # 触发停车进行中（外部注入）
        self.stop_flag = False

    def step(self, t):
        """t: 绝对时间 ms。返回 (state, speed)"""
        progress = t - self.state_tick
        if self.state == IDLE:
            if t >= A:
                self.state = RAMPUP
                self.state_tick = t
        elif self.state == RAMPUP:
            sp = TARGET * progress // B
            if sp >= TARGET:
                sp = TARGET
                self.state = RUN
                self.state_tick = t
            self.speed = min(sp, 999)
        elif self.state in (RUN, SLOW):
            if self.triggered:
                self.triggered = False
                self.state = STOPPING
                self.state_tick = t
                self.ramp_start = self.speed
                return
            if E <= t < E + G:
                if self.state != SLOW:
                    self.state = SLOW
                self.speed = TARGET * F // 100
            else:
                if self.state != RUN:
                    self.state = RUN
                self.speed = TARGET
            if t >= self.stop_time or t >= MAX_RUN:
                self.state = STOP
                self.speed = 0
        elif self.state == STOPPING:
            if progress >= H:
                self.state = WAIT
                self.state_tick = t
                self.speed = 0
            else:
                remain = H - progress
                self.speed = self.ramp_start * remain // H
        elif self.state == WAIT:
            if progress >= I:
                self.state = RAMPUP
                self.state_tick = t
        elif self.state == STOP:
            self.speed = 0
        return

    def state_name(self):
        return ["IDLE","RAMPUP","RUN","SLOW","STOPPING","WAIT","STOP"][self.state]

# ---- 测试框架 ----
PASS = 0
FAIL = 0
def check(name, cond, detail=""):
    global PASS, FAIL
    if cond: PASS += 1
    else:
        FAIL += 1
        print(f"FAIL: {name} {detail}")

def run_to(m, t_end, dt=10, trigger_at=None):
    """从上次位置步进仿真到 t_end；trigger_at 为触发时刻列表"""
    start = getattr(m, 'now', 0)
    for t in range(start, t_end + 1, dt):
        if trigger_at and t in trigger_at:
            m.triggered = True
        m.step(t)
        m.now = t
    return (t_end, m.state, m.speed)

def test_normal_timing():
    m = Motor(600_000)
    # t=1s：仍 IDLE
    m.step(1000); check("晚启动前 IDLE", m.state == IDLE)
    # t=2s：进入 RAMPUP
    m.step(2000); check("A 秒进入 RAMPUP", m.state == RAMPUP, f"state={m.state_name()}")
    # t=4s（缓启动中点）：速度≈一半
    m.step(4000); check("缓启动中点速度", m.speed == TARGET*2000//4000, f"speed={m.speed}")
    # t=6s：RUN 全速
    m.step(6000); check("缓启动结束 RUN", m.state == RUN and m.speed == TARGET)
    # t=42s：SLOW 50%
    m.step(42000); check("E 秒进入 SLOW", m.state == SLOW and m.speed == 499, f"speed={m.speed}")
    # t=46.9s：仍在窗口
    m.step(46900); check("窗口内保持降速", m.state == SLOW and m.speed == 499)
    # t=47s：恢复
    m.step(47000); check("G 秒后恢复 RUN", m.state == RUN and m.speed == TARGET, f"state={m.state_name()}")

def test_trigger_stop_sequence():
    m = Motor(600_000)
    run_to(m, 30_000)                     # 正常运行到 30s
    check("触发前 RUN", m.state == RUN)
    m.triggered = True; m.step(30_000)
    check("触发进入 STOPPING", m.state == STOPPING)
    # 1s 后：减速中
    m.step(31_000)
    check("减速中途", m.state == STOPPING and 0 < m.speed < 999, f"speed={m.speed}")
    # 2s 后：WAIT
    m.step(32_000)
    check("H 秒后 WAIT", m.state == WAIT and m.speed == 0)
    # 12s 后：RAMPUP（重启，不再经晚启动 A）
    m.step(42_000)
    check("I 秒后重启缓启动", m.state == RAMPUP, f"state={m.state_name()}")
    # 重启 2s（t=44s）：速度应为 999*2000/4000=499（无 A 延时）
    m.step(44_000)
    check("重启缓启动速度(无晚启动)", m.speed == TARGET*2000//B, f"speed={m.speed}")
    # 重启 4s（t=46s）：RUN 全速
    m.step(46_000)
    check("重启后 RUN", m.state == RUN and m.speed == TARGET, f"state={m.state_name()}, speed={m.speed}")

def test_stop_time_absolute():
    """触发重启后若 t≥stop_time → 进 RUN 的下一拍立即 STOP（绝对计时边界行为）"""
    m = Motor(40_000)                     # stop_time=40s（>触发时刻）
    run_to(m, 30_000)                     # 正常运行到 30s（RUN 状态）
    m.triggered = True; m.step(30_000)    # 触发
    run_to(m, 42_000)                     # 序列 30-42s 结束，重启缓启动
    check("停车序列完整走完", m.state == RAMPUP, f"state={m.state_name()}")
    run_to(m, 47_000)                     # 缓启动 42-46s，46s 进 RUN 后检查 stop_time（40s 已过）
    check("绝对计时到时 STOP", m.state == STOP and m.speed == 0,
          f"state={m.state_name()}, t=47s stop_time=40s")

def test_potentiometer_stop():
    m = Motor(60_000)                     # 60s
    run_to(m, 60_000)
    check("到时 STOP", m.state == STOP and m.speed == 0)

def test_max_run_time():
    m = Motor(10_000_000)                 # 远大于上限
    run_to(m, 1_000_000)
    check("1000s 上限 STOP", m.state == STOP)

def test_stop_sequence_no_stop_time_check():
    """停车序列期间不检查 stop_time（序列完整走完），重启后进 RUN 才 STOP"""
    m = Motor(33_000)                     # stop_time=33s——落在停车序列期间
    run_to(m, 30_000)
    m.triggered = True; m.step(30_000)    # 触发 → STOPPING(30-32s)
    run_to(m, 40_000)                     # 33s 已过 stop_time，但序列中（WAIT 32-42s）
    check("序列期间不被打断", m.state == WAIT, f"state={m.state_name()}")
    run_to(m, 47_000)                     # 重启缓启动 42-46s，46s 进 RUN 检查 → STOP
    check("序列后立即 STOP", m.state == STOP, f"state={m.state_name()}")

def test_trigger_before_e_window_restart_inside():
    """触发在窗口前，重启后落入窗口：窗口未结束继续降速（绝对时间语义）"""
    m = Motor(600_000)
    run_to(m, 30_000)
    m.triggered = True; m.step(30_000)
    run_to(m, 46_200)                     # 重启后 t=46.2s ∈ [42,47)：RUN 帧内检查窗口
    check("重启后窗口内 SLOW", m.state == SLOW and m.speed == 499, f"state={m.state_name()}, speed={m.speed}")
    run_to(m, 48_000)
    check("窗口结束恢复 RUN", m.state == RUN and m.speed == TARGET,
          f"state={m.state_name()}, speed={m.speed}")

if __name__ == "__main__":
    print("=== 电机状态机仿真测试 ===")
    test_normal_timing()
    test_trigger_stop_sequence()
    test_stop_time_absolute()
    test_potentiometer_stop()
    test_max_run_time()
    test_stop_sequence_no_stop_time_check()
    test_trigger_before_e_window_restart_inside()
    print(f"通过 {PASS}，失败 {FAIL}")
    raise SystemExit(1 if FAIL else 0)
