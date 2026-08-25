# WiFi 参数配置模式实施计划

> 目标：5 秒内连续通断电 3 次进入配置模式 → 板子开启 WiFi 热点 → 电脑连接后打开网页，
> 直接修改系统参数（时间统一用秒、触发词直接输中文自动转 GBK）→ 保存重启生效。
> 全程无需重新烧录固件。

---

## 第 0 步：前置验证——LED 颜色流转（约 10 分钟）

commit `89d3324` 的 LED 优先级修复已编译、未烧录。开工先烧录并刷卡验证：

刷卡"太阳"后应看到完整流转：
🟢绿(刷卡) → 🟡黄(STOPPING 2秒) → 🔴红(WAIT 10秒) → 🟠橙(RAMPUP 4秒) → ⚪白(RUN)

- ✅ 流转正确 → 进入阶段一
- ❌ 仍是绿→白直跳 → 先排查（检查 Motor_IsInStopSequence() 返回值与调用时序），通过后再开始本计划

---

## 阶段一：GBK⇄UTF-8 转换模块（预计 1~2 小时）

1. 新增 `components/common/gbk_utf8.c/h`
   - GB2312 Unicode 映射表（const 放 Flash，约 15KB）：一级 3755 字 + 二级 3008 字 + 常用符号
   - `int gbk_to_utf8(const uint8_t *gbk, size_t len, char *out, size_t out_size);`
   - `int utf8_to_gbk(const char *utf8, size_t len, uint8_t *out, size_t out_size);`（查表二分搜索）
   - ASCII 直通；表外生僻字降级输出 '?'（不中断）
2. 替换现有硬编码：
   - `rfid_task.c` / `rfid_process.c` 删除太阳/地球两条硬编码查找表，改调 `gbk_to_utf8()`
   - 效果：任何卡内容都能在串口正确显示中文，不再限于预设词
3. 主机单测：
   - 新增 `esp32/tests/test_gbk_utf8.c` 加入 `run_tests.sh`
   - 用例：太阳/地球往返转换、ASCII 混合、空串、表外字符降级、缓冲区不足防护
4. 编译两版 + 烧录，串口刷卡验证中文显示

## 阶段二：运行时参数层（核心重构，预计半天）

1. 新增 `components/common/nvs_params.c/h`
2. 定义 `params_t` 运行时结构（网页可配的全部参数）：

   | 分组 | 字段（内部 ms/原始值） | 默认值=现 config.h 宏 |
   |---|---|---|
   | 电机 | late_ms, slow_ms, target_speed | 2000/4000/999 |
   | 减速窗口 | slowwins[8]{start_ms,dur_ms,pct} + count（**多段减速**；不重叠、按时间升序；空列表=无降速） | 默认 1 条 {42s,5s,50%}=现行为 |
   | 电机转向 | motor_dir：0=正转，1=反转 | 0 |
   | 触发词 | rules[8]{gbk[16],len,count_req,speak_en}, count_interval_ms, stop_ramp_ms, wait_ms | 太阳(1次)/地球(2次)等 |
   | 其他 | led_on_ms, dedup_ms, rfid_poll_ms, read_timeout_ms | 3000/10000/800/20 |

3. API：
   - `params_init()`：NVS 有值用 NVS，无值回落 config.h 默认宏（首次烧录零感知）
   - `params_save()`：范围校验后写 NVS
   - `extern params_t g_params;` 全局单例，业务代码只读它
4. **重构引用点（宏 → g_params.xxx）**：
   - `motor_logic.c`（A/B/E/F/G 时序）、`rfid_logic.c`（TRIGGER_RULES 数组→运行时数组指针、去重窗口、计数间隔）
   - `rfid_task.c`/`rfid_process.c`（C/D/轮询/超时）、`motor_task.c`/`motor_process.c`
   - ⚠️ `RfidLogic_RuleCount()` 等接口同步适配运行时数量
   - ⚠️ 同步更新 `esp32/tests`（单测可注入自定义参数做验证）
5. **小步走纪律**：每改一个引用点立即编译 + 跑 `run_tests.sh`，默认值下行为必须与改造前完全一致

## 阶段三：进入配置模式检测（预计 1 小时）

1. 新增 `components/common/config_mode.c/h`
2. 方案（跨断电计时的工程等效）：
   - 每次上电：读 NVS `boot_count`，+1 后立刻写回
   - 持续运行超 10 秒 → 自动清零计数器
   - 计数 ≥ 3（连按 3 次 RST 各产生一次重启；或快速通断电累计）→ 进入配置模式
   - 说明：断电后 RTC 不保电，无法测真实间隔；用"短时运行即计数"等效"短时间内连续重启"
3. **触发手势（定稿）：连按 3 次 RST**——EN 引脚复位与断电完全等同（全片复位、NVS 均保留）；
   每次 RST/上电计数 +1，计满 3 触发（运行中的板子无需预置计数）；快速通断电同样有效
4. 防误触：`esp_reset_reason()` 过滤——仅 `ESP_RST_POWERON` 累计计数；
   `ESP_RST_PANIC`（固件崩溃自动重启）不累计，避免程序崩溃循环误入配置模式
3. 进入时 RGB LED 蓝色快闪提示；30 秒无人连接自动退出并恢复正常启动
4. 正常模式代价：每次上电多一次 NVS 读写（微秒级），无感

## 阶段四：WiFi AP + HTTP 服务 + 网页（预计半天）

1. `wifi_ap.c/h`：SSID=`EV-Car-Setup`，无密码（或 12345678），IP `192.168.4.1`，DHCP 开启
2. `web_server.c/h`（esp_http_server）：
   | 路由 | 功能 |
   |---|---|
   | GET / | 返回配置页面 |
   | GET /api/params | 当前参数 JSON（触发词经 gbk_to_utf8 转中文回显）|
   | POST /api/params | JSON 提交（触发词经 utf8_to_gbk 转换），范围校验后 params_save() |
   | POST /api/restart | 保存并重启板子 |
3. `webpage.h`：单文件内嵌 HTML+CSS+JS（压缩后约 10KB）
   - 所有时间字段单位=**秒**（支持一位小数）；加载时 ms→s，提交时 s→ms
   - 电机转向=下拉框（正转/反转）
   - 减速窗口=可增删列表行 `[第几秒][持续秒][百分比]` + "添加窗口"按钮；JS 校验不重叠/范围，保存时自动按时间排序
   - 触发词=文本框直接输汉字（如"太阳"），旁边实时预览对应 GBK hex
   - 提交前 JS 先做一轮范围预校验
4. sdkconfig/CMakeLists：启用 nvs_flash、esp_wifi、esp_http_server

## 阶段五：集成验收清单

- [ ] 通断电 3 次 → LED 蓝色快闪 → 出现 EV-Car-Setup 热点
- [ ] 连接后浏览器打开 192.168.4.1 显示配置页
- [ ] 当前参数正确回显，触发词显示中文
- [ ] 把静止等待 10→20 秒，保存重启，刷卡实测停车等待变成 20 秒
- [ ] 新增触发词（如"火星"，卡 block4 写入对应 GBK），刷卡命中并停车播报
- [ ] 非配置模式下原有行为全部不回归：电机时序/读卡/播报/LED 流转
- [ ] 两版编译通过 + 单测全绿 + 漂移检查 OK

---

## 风险与注意

| 风险 | 对策 |
|---|---|
| 阶段二重构量大（共享逻辑层宏→结构体） | 小步走：一次改一个文件，改完即编译+跑单测 |
| 多段减速窗口改 motor_logic 状态机 | 单区间判断→遍历窗口数组；SLOW 仅从 RUN 进入、窗口结束回 RUN；STOPPING/WAIT/RAMPUP 优先级更高（窗口被覆盖即跳过）；单测补多窗口用例 |
| motor_drv.c 当前差分 PWM 仅正转 | 新增方向支持：Motor_Control 内按 g_params.motor_dir 交换 GPIO4/GPIO5 角色（反转=PWM1 拉低、PWM2 输出速度），上电按参数初始化一次即可 |
| motor_drv.c 当前差分 PWM 仅正转 | 新增方向支持：Motor_Control 内按 g_params.motor_dir 交换 GPIO4/GPIO5 角色（反转=PWM1 拉低、PWM2 输出速度），上电按参数初始化一次即可 |
| TRIGGER_RULES 编译期→运行期 | rfid_logic 内部静态表改指针注入，RuleCount 接口同步 |
| WiFi 内存峰值 ~50KB | S3 512KB SRAM，充裕 |
| NVS 首次初始化 | nvs_flash_init 处理 ESP_ERR_NVS_NO_FREE_PAGES（erase retry） |
| usbip 串口不稳定影响验证 | 尽量用自己的串口软件看日志；烧录用 BOOT+RST 手动进下载模式 |

## 进度预期

一天完成阶段一、二、三问题不大；阶段四、五视进度可能顺延到第三天。
建议顺序严格执行：转换模块先行（独立可验证）→ 参数层（地基）→ 配置检测 → WiFi/网页。

## 新增/修改文件总览

新增：
- components/common/gbk_utf8.c/h
- components/common/nvs_params.c/h
- components/common/config_mode.c/h
- components/common/wifi_ap.c/h
- components/common/web_server.c/h
- components/common/webpage.h
- esp32/tests/test_gbk_utf8.c、test_nvs_params.c

修改：
- components/common/config.h（宏保留，转为默认值来源）
- components/common/CMakeLists.txt（新源文件 + REQUIRES nvs_flash esp_wifi esp_http_server）
- rfid_task.c / rfid_process.c / motor_task.c / motor_process.c / app_main.c（两版，参数化 + 配置模式入口）
- esp32/tests/run_tests.sh（加新测试）
