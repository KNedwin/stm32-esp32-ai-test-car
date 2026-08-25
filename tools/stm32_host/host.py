#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""新能源小车 STM32 上位机：浏览器参数配置页 ⇄ USART3 命令行桥 + 编译/烧录工具。

用法：
    python host.py                 # 默认 http://127.0.0.1:8321 并自动开浏览器
    python host.py --port 9000     # 换端口
    python host.py --distro Ubuntu-26.04   # Windows 下 wsl.exe 编译用发行版名

依赖：pip install pyserial；烧录另需 st-flash(ST-Link) 或 stm32flash(ISP)。
"""
import argparse
import json
import platform
import subprocess
import sys
import threading
import time
import webbrowser
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path

try:
    import serial
    import serial.tools.list_ports
except ImportError:
    print("缺少 pyserial：请先执行  pip install pyserial")
    sys.exit(1)

HOST_ROOT = Path(__file__).resolve().parent          # tools/stm32_host
REPO_ROOT = HOST_ROOT.parent.parent                  # 仓库根
PAGE_FILE = HOST_ROOT / "page.html"

PROJECTS = {
    "rtos":      "stm32/rfid_tts_rtos",
    "baremetal": "stm32/rfid_tts_baremetal",
}
BIN_NAMES = {
    "rtos":      "rfid_tts_rtos.bin",
    "baremetal": "rfid_tts_baremetal.bin",
}


# ---------------------------------------------------------------- 串口桥 ----
class SerialBridge:
    """行协议事务：发送一条命令，收集以 '>' 开头的应答行，直到静默。"""

    def __init__(self):
        self.ser = None
        self.lock = threading.Lock()

    @property
    def is_open(self):
        return self.ser is not None and self.ser.is_open

    def open(self, port, baud=115200):
        with self.lock:
            self.close_nolock()
            self.ser = serial.Serial(port, baud, timeout=0.05)
            time.sleep(0.2)                       # 让板内残留输出排空
            self.ser.reset_input_buffer()

    def close(self):
        with self.lock:
            self.close_nolock()

    def close_nolock(self):
        if self.ser is not None:
            try:
                self.ser.close()
            except Exception:
                pass
            self.ser = None

    def transact(self, cmd, quiet=0.30, max_wait=3.0):
        """返回本次命令的全部输出行（str 列表）。
        收包策略：读到数据后，再连续 quiet 秒无新数据才算结束，
        保证 GET win / GET rule 的多行应答完整。"""
        if not self.is_open:
            raise RuntimeError("串口未连接")
        with self.lock:
            self.ser.reset_input_buffer()
            self.ser.write((cmd + "\r\n").encode("ascii"))
            buf = b""
            start = time.time()
            last_data = time.time()
            while time.time() - start < max_wait:
                n = self.ser.in_waiting
                if n:
                    buf += self.ser.read(n)
                    last_data = time.time()
                elif buf and (time.time() - last_data) >= quiet:
                    break                          # 应答已静默，收包完成
                else:
                    time.sleep(0.01)
            text = buf.decode("utf-8", errors="replace")
            return [ln.strip() for ln in text.splitlines() if ln.strip()]

    def cli(self, cmd):
        """执行一条 CLI 命令，返回去掉 '> ' 前缀的应答行列表。"""
        lines = self.transact(cmd)
        resp = [ln[2:] if ln.startswith("> ") else ln for ln in lines]
        if not resp:
            raise RuntimeError(f"命令无应答: {cmd}")
        return resp


BRIDGE = SerialBridge()


# ---------------------------------------------------------------- 参数转换 ----
def parse_params():
    """GET all/win/rule 三条命令拼成与 ESP32 网页同构的 JSON。"""
    scalars = {}
    for ln in BRIDGE.cli("GET all"):
        for kv in ln.strip("{} ").split(","):
            if ":" in kv:
                k, v = kv.split(":", 1)
                k = k.strip('"')
                try:
                    scalars[k] = float(v) if "." in v else int(v)
                except ValueError:
                    pass

    wins = []
    for ln in BRIDGE.cli("GET win"):
        p = ln.split()
        if len(p) == 5 and p[0] == "W":
            wins.append({"start_ms": int(float(p[2]) * 1000),
                         "dur_ms": int(float(p[3]) * 1000), "pct": int(p[4])})

    rules = []
    for ln in BRIDGE.cli("GET rule"):
        p = ln.split()
        if len(p) == 4 and p[0] == "R":
            word = bytes.fromhex(p[2]).decode("gbk", errors="replace").rstrip("\x00")
            rules.append({"text": word, "count": int(p[3]), "speak": int(p[4])})

    return {
        "late_ms": scalars.get("late_s", 0) * 1000,
        "slow_ms": scalars.get("slow_s", 0) * 1000,
        "target_speed": scalars.get("target", 999),
        "motor_dir": scalars.get("dir", 0),
        "stop_ramp_ms": scalars.get("sr_s", 0) * 1000,
        "wait_ms": scalars.get("ws_s", 0) * 1000,
        "led_on_ms": scalars.get("led_s", 0) * 1000,
        "dedup_ms": scalars.get("dedup_s", 0) * 1000,
        "rfid_poll_ms": scalars.get("poll_s", 0) * 1000,
        "count_interval_ms": scalars.get("ci_s", 0) * 1000,
        "autostop_ms": scalars.get("as_s", 300000),
        "slowwins": wins,
        "rules": rules,
    }


def apply_params(body):
    """网页载荷 -> CLI 命令序列逐条下发并核对 OK。"""
    seq = []
    keymap = [("late_s", "late_s"), ("slow_s", "slow_s"), ("target_speed", "target"),
              ("motor_dir", "dir"), ("stop_ramp_s", "sr_s"), ("wait_s", "ws_s"),
              ("led_on_s", "led_s"), ("dedup_s", "dedup_s"), ("poll_s", "poll_s"),
              ("count_interval_s", "ci_s"), ("autostop_s", "as_s")]
    for jkey, ckey in keymap:
        if jkey in body:
            seq.append(f"SET {ckey} {body[jkey]}")

    # 先查现有数量再清空重建（避免残留旧窗口/旧触发词）
    cur = {}
    for ln in BRIDGE.cli("GET all"):
        for kv in ln.strip("{} ").split(","):
            if ":" in kv:
                k, v = kv.split(":", 1)
                k = k.strip('"')
                try:
                    cur[k] = int(v)
                except ValueError:
                    pass
    for i in reversed(range(cur.get("win_n", 0))):
        seq.append(f"SET win_del {i}")
    for i in reversed(range(cur.get("rule_n", 0))):
        seq.append(f"SET rule_del {i}")
    for w in body.get("slowwins", []):
        seq.append(f"SET win_add {w['start_s']} {w['dur_s']} {int(w['pct'])}")
    for r in body.get("rules", []):
        gbk = r["text"].encode("gbk").hex()
        seq.append(f"SET rule_add {gbk} {int(r['count'])} {int(r['speak'])}")
    seq.append("SAVE")

    for cmd in seq:
        resp = " ".join(BRIDGE.cli(cmd))
        if "OK" not in resp.upper():
            return False, f"{cmd} -> {resp}"
    return True, f"已下发 {len(seq)} 条命令并写入 Flash"


# ---------------------------------------------------------------- 后台作业 ----
class Job:
    def __init__(self):
        self.lock = threading.Lock()
        self.log = []
        self.running = False
        self.thread = None

    def append(self, text):
        with self.lock:
            self.log.extend(text.splitlines())
            del self.log[:-2000]

    def start(self, fn, *a):
        with self.lock:
            if self.running:
                return False
            self.log = []
            self.running = True
        self.thread = threading.Thread(target=self._wrap, args=(fn,) + a, daemon=True)
        self.thread.start()
        return True

    def _wrap(self, fn, *a):
        try:
            fn(*a)
        finally:
            self.running = False

    def snapshot(self):
        with self.lock:
            return {"running": self.running, "log": self.log[-400:]}


JOB = Job()


def _stream_run(cmd, cwd=None, shell=False):
    JOB.append("$ " + " ".join(cmd) if not shell else "$ " + cmd)
    try:
        p = subprocess.Popen(cmd, cwd=cwd, shell=shell,
                             stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                             text=True, errors="replace")
        for line in p.stdout:
            JOB.append(line.rstrip())
        rc = p.wait()
        JOB.append(f"[退出码 {rc}] " + ("成功" if rc == 0 else "失败"))
    except FileNotFoundError as e:
        JOB.append(f"找不到可执行文件: {e.filename}（见 README 依赖说明）")


def wsl_posix_root(args):
    """Windows 下把仓库路径翻译为 WSL 内部路径；Linux 直接用本地路径。"""
    if platform.system() != "Windows":
        return str(REPO_ROOT)
    conv = subprocess.run(
        ["wsl.exe", "-d", args.distro, "wslpath", "-u", str(REPO_ROOT)],
        capture_output=True, text=True, errors="replace")
    path = (conv.stdout or "").strip()
    if conv.returncode != 0 or not path.startswith("/"):
        raise RuntimeError("无法把仓库路径转换为 WSL 路径，请用 --wsl-root 手工指定")
    return path


def do_build(args, target):
    try:
        root = wsl_posix_root(args)
    except RuntimeError as e:
        JOB.append(str(e))
        return
    proj = f"{root}/{PROJECTS[target]}"
    inner = f"cd '{proj}' && cmake --build build/Debug"
    if platform.system() == "Windows":
        _stream_run(["wsl.exe", "-d", args.distro, "--cd", proj,
                     "bash", "-lc", inner])
    else:
        _stream_run(["bash", "-lc", inner])


def do_flash_stlink(args, target):
    try:
        root = wsl_posix_root(args)
    except RuntimeError as e:
        JOB.append(str(e))
        return
    bin_path = f"'{root}/{PROJECTS[target]}/build/Debug/{BIN_NAMES[target]}'"
    if platform.system() == "Windows":
        _stream_run(["wsl.exe", "-d", args.distro, "bash", "-lc",
                     f"st-flash --connect-under-reset write {bin_path} 0x08000000"])
    else:
        _stream_run(["st-flash", "--connect-under-reset", "write",
                     f"{PROJECTS[target]}/build/Debug/{BIN_NAMES[target]}",
                     "0x08000000"], cwd=str(REPO_ROOT))


def do_flash_isp(args, target):
    if not BRIDGE.is_open:
        JOB.append("ISP 烧录需先在页面顶部连接串口（CLI 走 USART3）")
        return
    JOB.append(">> 发送 ISP 命令，板子将软跳系统 Bootloader…")
    try:
        BRIDGE.cli("ISP")
    except Exception as e:
        JOB.append(f"(跳转中断线属正常现象: {e})")
    BRIDGE.close()
    time.sleep(0.5)
    port = ARGS.last_port
    bin_win = rf"\\wsl$\{args.distro}\{REPO_ROOT}\{PROJECTS[target]}\build\Debug\{BIN_NAMES[target]}" \
        if platform.system() == "Windows" \
        else str(REPO_ROOT / PROJECTS[target] / "build/Debug" / BIN_NAMES[target])
    if platform.system() == "Windows":
        _stream_run(["stm32flash", "-w", bin_win, "-v", "-g", "0x0", port])
    else:
        _stream_run(["stm32flash", "-w", bin_win, "-v", "-g", "0x0", port])


# ---------------------------------------------------------------- HTTP ----
ARGS = None  # main() 里赋值


class Handler(BaseHTTPRequestHandler):
    def log_message(self, *a):      # 静默默认访问日志
        pass

    def _send(self, code, ctype, body):
        self.send_response(code)
        self.send_header("Content-Type", ctype)
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def _json(self, obj, code=200):
        self._send(code, "application/json; charset=utf-8",
                   json.dumps(obj, ensure_ascii=False).encode("utf-8"))

    def _post_body(self):
        n = int(self.headers.get("Content-Length") or 0)
        return self.rfile.read(n).decode("utf-8") if n else ""

    def do_GET(self):
        if self.path in ("/", "/index.html"):
            self._send(200, "text/html; charset=utf-8", PAGE_FILE.read_bytes())
        elif self.path == "/api/ports":
            ports = [{"name": p.device, "desc": p.description}
                     for p in serial.tools.list_ports.comports()]
            self._json({"ports": ports})
        elif self.path == "/api/params":
            try:
                self._json(parse_params())
            except Exception as e:
                self._json({"error": str(e)}, 502)
        elif self.path == "/api/job":
            self._json(JOB.snapshot())
        else:
            self._json({"error": "not found"}, 404)

    def do_POST(self):
        body = self._post_body()
        try:
            data = json.loads(body) if body else {}
        except json.JSONDecodeError:
            self._json({"error": "bad json"}, 400)
            return

        if self.path == "/api/connect":
            ARGS.last_port = data.get("port", "")
            try:
                BRIDGE.open(ARGS.last_port, ARGS.baud)
                self._json({"ok": True})
            except Exception as e:
                self._json({"ok": False, "error": str(e)}, 502)
        elif self.path == "/api/disconnect":
            BRIDGE.close()
            self._json({"ok": True})
        elif self.path == "/api/params":
            try:
                ok, msg = apply_params(data)
                self._json({"ok": ok, "msg": msg} if ok else {"error": msg}, 200 if ok else 502)
            except Exception as e:
                self._json({"error": str(e)}, 502)
        elif self.path == "/api/reboot":
            try:
                BRIDGE.cli("REBOOT")
                self._json({"ok": True})
            except Exception as e:
                self._json({"error": str(e)}, 502)
        elif self.path == "/api/cli":          # 原始命令透传（调试 DUMP 等）
            try:
                self._json({"lines": BRIDGE.cli(str(data.get("cmd", ""))[:64])})
            except Exception as e:
                self._json({"error": str(e)}, 502)
        elif self.path == "/api/job":
            kind, target = data.get("kind"), data.get("target", "rtos")
            if target not in PROJECTS:
                self._json({"error": "bad target"}, 400)
                return
            started = False
            if kind == "build":
                started = JOB.start(do_build, ARGS, target)
            elif kind == "flash_stlink":
                started = JOB.start(do_flash_stlink, ARGS, target)
            elif kind == "flash_isp":
                started = JOB.start(do_flash_isp, ARGS, target)
            self._json({"ok": started} if started else {"error": "已有任务在运行"})
        else:
            self._json({"error": "not found"}, 404)


def main():
    global ARGS
    ap = argparse.ArgumentParser(description="新能源小车 STM32 上位机")
    ap.add_argument("--port", type=int, default=8321, help="HTTP 端口（默认 8321）")
    ap.add_argument("--baud", type=int, default=115200, help="USART3 波特率")
    ap.add_argument("--distro", default="Ubuntu-26.04", help="WSL 发行版名（Windows 编译用）")
    ap.add_argument("--wsl-root", dest="wsl_root", default="", help="手工指定仓库在 WSL 内的路径")
    ap.add_argument("--no-browser", action="store_true")
    ARGS = ap.parse_args()
    ARGS.last_port = ""

    srv = ThreadingHTTPServer(("127.0.0.1", ARGS.port), Handler)
    url = f"http://127.0.0.1:{ARGS.port}"
    print(f"上位机已启动: {url}  （Ctrl-C 退出）")
    print(f"仓库根目录 : {REPO_ROOT}")
    if not ARGS.no_browser:
        threading.Timer(0.6, lambda: webbrowser.open(url)).start()
    try:
        srv.serve_forever()
    except KeyboardInterrupt:
        print("\n已退出")
        BRIDGE.close()


if __name__ == "__main__":
    main()
