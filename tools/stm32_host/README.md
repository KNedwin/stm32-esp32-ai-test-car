# STM32 上位机（tools/stm32_host）

浏览器图形界面 ⇄ 板子 USART3 命令行（param_cli）的桥接工具，附带编译/烧录按钮。
与 ESP32 网页配置页同构：同一套表单、同一套参数语义，PC 侧负责中文→GBK 编码。

## 文件

| 文件 | 作用 |
|---|---|
| host.py | 入口：内置 HTTP 服务(默认 127.0.0.1:8321) + pyserial 串口事务桥 |
| page.html | 配置页（复用 ESP32 表单，增加串口连接栏与编译/烧录工具区） |

## 依赖安装

- Python 3.8+，`pip install pyserial`
- 烧录二选一：
  - ST-Link：WSL 内 `st-flash`（stlink-tools）——Windows 下脚本自动经 wsl.exe 调用
  - ISP 串口烧录：`stm32flash`（Windows 版或 WSL 内均可）

## 运行

```bash
python host.py                    # 自动开浏览器 http://127.0.0.1:8321
python host.py --port 9000        # 换端口
python host.py --distro Ubuntu    # Windows 下指定 wsl.exe 发行版名
python host.py --no-browser      # 不自动开浏览器
```

推荐在 **Windows 本机**运行（串口直连 COMx 最稳）；在 WSL 内运行则串口走 /dev/ttyUSB*（需 usbipd 转发）。

## 操作流程

1. **连接**：页面顶部选串口 → 连接（CLI 波特率固定 115200）
2. **读参数**：连接成功后自动 GET all / GET win / GET rule 填充表单
3. **改参数**：直接编辑 → 💾 保存并即时生效（逐条 SET 下发 + SAVE 写 Flash，
   固件每条命令后即时重新注入逻辑层，无需重启）
4. **重启验证**：🔄 重启板子 → 重连读取，核对参数保持（阶段五验收项）
5. **编译**：选目标工程 → 🔨 编译（等同 `cmake --build build/Debug`，日志实时回显）
6. **烧录**：
   - ST-Link：一键 `st-flash --connect-under-reset write <bin> 0x08000000`
   - ISP 串口：先连串口 → 点按钮。脚本先发 `ISP` 命令软跳系统 Bootloader，
     再调 stm32flash 写入。⚠️ **跳转前拔掉 U13T 读卡模块**（其 RX 占用 USART1 干扰
     Bootloader 应答），烧完插回。

## API 一览（调试用）

| 端点 | 说明 |
|---|---|
| GET  /api/ports | 枚举本机串口 |
| POST /api/connect {port} | 打开串口 |
| GET  /api/params | 读板内参数（ms 字段，触发词已转 UTF-8 文本） |
| POST /api/params | 下发参数（秒字段 + rules.text 中文，PC 侧转 GBK hex） |
| POST /api/reboot | 发 REBOOT 复位 |
| POST /api/cli {cmd} | 原始 CLI 命令透传（如 DUMP） |
| GET  /api/job · POST /api/job | 编译/烧录后台作业状态与日志 |

## 故障排查

- **连接失败/无应答**：确认板子 USART3 (PB10/PB11) 接 USB-TTL，TX/RX 未接反；波特率 115200。
- **保存时某条命令 ERR:full**：窗口/触发词最多各 8 组，先删后加。
- **ISP 后 stm32flash 找不到端口**：板子跳 Bootloader 后枚举的仍是同一 COM 口；
  若被上位机占用会失败——脚本已自动断开，检查其他串口监视器是否关闭。
- **编译找不到 wsl/路径**：Windows 运行时用 `--distro` 指定发行版；特殊路径用 `--wsl-root` 手工指定仓库在 WSL 内的位置。
