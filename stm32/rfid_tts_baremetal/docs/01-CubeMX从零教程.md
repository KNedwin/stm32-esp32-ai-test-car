# 裸机版 - STM32CubeMX 从零教程（超详细版）

> 阅读对象：完全没接触过 STM32CubeMX 的读者。请从头到尾按顺序操作，每一步都不要跳过。
> 本教程生成的是 **裸机版**（不使用操作系统，主循环 + 状态机）工程。
>
> ⚠️ 如果你用的是 RTOS 版，请阅读 `../rfid_tts_rtos/docs/01-CubeMX从零教程.md`。两版教程 90% 相同，**差异点已在本教程中用【裸机版差异】标出**。

---

## 目录

- [0. 准备工作：需要安装的软件](#0-准备工作需要安装的软件)
- [1. 新建工程](#1-新建工程)
- [2. 配置系统时钟（RCC）](#2-配置系统时钟rcc)
- [3. 配置调试接口与系统（SYS）](#3-配置调试接口与系统sys)
- [4. 配置外部晶振时钟树](#4-配置外部晶振时钟树)
- [5. 配置 GPIO 输出引脚（LED）](#5-配置-gpio-输出引脚led)
- [6. 配置串口 1（读卡模块 USART1）](#6-配置串口-1读卡模块-usart1)
- [7. 配置串口 2（语音模块 USART2）](#7-配置串口-2语音模块-usart2)
- [8. 配置串口 3（数据输出 USART3）](#8-配置串口-3数据输出-usart3)
- [9. 配置定时器 TIM2（电机 PWM）](#9-配置定时器-tim2电机-pwm)
- [10. 配置 ADC1（电位器）](#10-配置-adc1电位器)
- [11. 为什么本版本不需要 FreeRTOS](#11-为什么本版本不需要-freertos)
- [12. 配置 NVIC 中断优先级](#12-配置-nvic-中断优先级)
- [13. 工程设置（Project Manager）](#13-工程设置project-manager)
- [14. 生成代码](#14-生成代码)
- [15. 生成后必须手动补充的代码](#15-生成后必须手动补充的代码)
- [16. 编译与烧录](#16-编译与烧录)
- [17. 常见问题 FAQ](#17-常见问题-faq)
- [附录：本教程全部配置速查表](#附录本教程全部配置速查表)

---

## 0. 准备工作：需要安装的软件

### 0.1 必须安装的软件

| 软件 | 用途 | 获取方式 |
|---|---|---|
| STM32CubeMX | 图形化配置芯片外设并生成工程 | ST 官网搜索 "STM32CubeMX" 下载，或从 www.st.com 的 Design Tools 页面获取 |
| 编译工具链 | 把 C 代码编译成机器码 | 二选一：Windows 装 Keil MDK 或 STM32CubeIDE；Linux 装 arm-none-eabi-gcc（本机已装） |
| ST-Link 驱动 | 烧录/调试器 | ST 官网搜索 "ST-LINK driver"；用 CMSIS-DAP 仿真器则无需 |

### 0.2 概念解释（给零基础读者）

- **STM32CubeMX**：ST 公司出的图形化配置工具。你在图形界面上"点选"用哪些引脚、什么参数，它自动生成 C 语言代码（这就是"生成工程"）。
- **Keil MDK / STM32CubeIDE / arm-none-eabi-gcc**：编译器。CubeMX 生成的代码需要编译器编译成 `.hex`/`.bin` 文件才能烧进芯片。
- **ST-Link / CMSIS-DAP**：烧录器，通过 USB 连接电脑和板子，把编译好的程序写进芯片。
- **裸机（Bare Metal）**：不装操作系统，程序只有一个 `main` 函数，在 `while(1)` 死循环里不停轮询/执行各功能模块。我们这次用的就是这种方式。

---

## 1. 新建工程

### 1.1 启动 STM32CubeMX

双击桌面图标启动 STM32CubeMX。首次启动会让你选择工作目录，直接点默认即可。启动后看到主界面，左侧是芯片列表/搜索框。

### 1.2 打开芯片选择器

在主界面点击：
```
File → New Project → 会弹出 "MCU Selector"（芯片选择器）窗口
```
如果弹窗提示升级固件包，选择 "Yes" 或稍后升级，不影响使用。

### 1.3 搜索并选择芯片

在 MCU Selector 窗口上方有一个搜索框（Search 输入框）：
1. 在搜索框输入：`STM32F103C8`
2. 在搜索结果列表中找到 **STM32F103C8Tx**（具体是 **STM32F103C8T6** 或列表中显示 STM32F103C8Tx 的那一行）
   - 怎么识别：芯片名下方会显示封装信息，选 **LQFP48**（48 引脚）
3. 点击选中该行（整行变蓝），然后点击右下角 **Start Project**（启动工程）

### 1.4 初始化默认选项

弹出窗口问 "Initialize all peripherals with their default mode?"（是否用默认模式初始化所有外设？）
→ 点 **Yes**（是）。

现在进入工程配置主界面（Pinout & Configuration 页）。中间是一张芯片引脚图，左侧是外设树，右侧是外设参数面板。

---

## 2. 配置系统时钟（RCC）

### 2.1 打开 RCC 配置

在左侧外设树中找到：
```
System Core（系统内核）→ RCC
```
点击 **RCC**，右侧会显示 RCC Mode and Configuration 面板。

### 2.2 开启外部高速晶振

在 "Mode"（模式）区域，找到 **High Speed Clock (HSE)**（高速外部时钟）这一行：
- 点击旁边的下拉框
- 选择 **Crystal/Ceramic Resonator**（晶体/陶瓷谐振器）

> 解释：我们的板子上有一颗 8MHz 晶振接在 PD0/PD1 引脚，HSE 就是使用这颗晶振。选这个选项后，引脚图上 PD0/PD1 会自动标记为 OSC_IN / OSC_OUT。

其余选项（LSI、LSE、MCO 等）保持默认，不用动。

---

## 3. 配置调试接口与系统（SYS）

### 3.1 打开 SYS 配置

左侧外设树：
```
System Core → SYS
```

### 3.2 开启 SWD 调试

在 "Debug"（调试）行：
- 下拉框选择 **Serial Wire**（串行线）

> 解释：Serial Wire 就是我们用的 SWD 调试口（PA13/PA14）。如果不选，芯片的 PA13/PA14 引脚不会被分配给调试器，烧录时会连不上。**这步忘了选会导致无法下载程序，务必做**。

### 3.3 时基（Timebase）设置 —— 【裸机版差异】保持 SysTick 默认即可

在 SYS 页面下方找到 **Timebase Source**（时基来源）：
- **保持默认的 SysTick，不要改动**

> 解释：
> - "时基"是 HAL 库用来计时的时钟源（`HAL_GetTick()` 返回的毫秒数就靠它）。
> - 裸机版没有操作系统，SysTick 空闲可用，直接当 HAL 时基最合适。
> - 只有 RTOS 版才需要把时基改成 TIM1（因为 FreeRTOS 要占用 SysTick）。本版本**不需要改**。

### 3.4 其他选项

`GPIO`、`NVIC` 等保持默认。

---

## 4. 配置外部晶振时钟树

### 4.1 打开时钟树页面

在软件界面顶部有一排标签页，点击：
```
Clock Configuration（时钟配置）
```
这里是一张时钟树图（方框连线图），中间是 PLL 和分频器，右侧显示最终各总线频率。

### 4.2 设置时钟来源与倍频

| 设置项 | 填写/选择 | 说明 |
|---|---|---|
| PLL Source（PLL 时钟源） | **HSE** | 用外部 8MHz 晶振做倍频来源 |
| PLL Mul（PLL 倍频数） | **x9** | 8MHz × 9 = 72MHz |
| SYSCLK（系统时钟） | 自动变为 72 | 不用手填，看到 72 就对了 |
| AHB Prescaler（AHB 分频） | **/1** | HCLK = 72MHz |
| APB1 Prescaler（APB1 分频） | **/2** | PCLK1 = 36MHz（TIM2 时钟仍为 72MHz） |
| APB2 Prescaler（APB2 分频） | **/1** | PCLK2 = 72MHz |
| ADC Prescaler（ADC 分频） | **/6** | ADC 时钟 = 12MHz（ADC 最高只能 14MHz） |

操作细节：
- 若看不到这些下拉框，先在时钟树左侧把 HSE 图标点成绿色（激活），把 PLL 图标点绿。
- 时钟树右上角有 "Apply"（应用）按钮，改完点击让它计算。
- 若某处变红字说明冲突，按上表逐项核对。

> 最终应看到：SYSCLK=72MHz、HCLK=72MHz、PCLK1=36MHz、PCLK2=72MHz、ADC=12MHz、TIM2 时钟 = 72MHz（在 APB1 下方）。

### 4.3 切回外设配置页面

配置完后点击顶部的：
```
Pinout & Configuration（引脚与外设配置）
```
回到引脚图页面，继续下面的配置。

---

## 5. 配置 GPIO 输出引脚（LED）

我们有三颗 LED 指示灯（低电平点亮），接在 **PC13、PB12、PA8** 三个引脚上。

### 5.1 在引脚图上配置 PC13

1. 在中间的芯片引脚图上，用鼠标左键找到 **PC13** 引脚（芯片右边缘附近，丝印 PC13 或 PC13-TAMPER-RTC）
2. 单击它，会弹出下拉菜单
3. 选择 **GPIO_Output**（GPIO 输出）

### 5.2 配置引脚参数（上拉、初始高电平）

1. 用相同方法把 **PB12** 和 **PA8** 都设置为 **GPIO_Output**
2. 现在点击左侧外设树中的：
   ```
   System Core → GPIO
   ```
   右侧会列出三个 GPIO 引脚（PC13、PB12、PA8），每一行对应一个引脚。
3. 对**每一个**引脚（三行都要设置），点击该行右侧的编辑按钮（铅笔图标），在弹出窗口中设置：

| 参数 | 选择 | 说明 |
|---|---|---|
| GPIO output level（输出初始电平） | **High（高）** | 上电时灯是灭的（低电平才亮） |
| GPIO mode（模式） | **Output Push Pull（推挽输出）** | 最常用的输出模式 |
| GPIO Pull-up/Pull-down（上下拉） | **Pull-up（上拉）** | 保证初始电平稳定 |
| Maximum output speed（最大输出速度） | **High（高速）** | 通用选择 |
| User Label（标签） | 可填 `LED1` / `LED2` / `LED3` | 生成代码里用这个名字（可选） |

设置完点 OK 关闭。

> 记住：**低电平点亮**。程序里 `LED_Sta(1)` 会把这三个脚拉低 → 灯亮；`LED_Sta(0)` 拉高 → 灯灭。

---

## 6. 配置串口 1（读卡模块 USART1）

### 6.1 开启 USART1

1. 左侧外设树点击：
   ```
   Connectivity（连接）→ USART1
   ```
2. 右侧 "Mode"（模式）区域，**Asynchronous（异步）** 旁边有下拉框：
   - 选择 **Asynchronous**（异步串口模式）
   - 芯片图上 PA9、PA10 会自动变成 USART1_TX、USART1_RX

> 解释：串口就是异步通信，"Asynchronous" 就是最普通的串口模式（TX 发、RX 收）。

### 6.2 设置串口参数

在右侧下方 **Parameter Settings（参数设置）** 标签里：

| 参数 | 填写 | 说明 |
|---|---|---|
| Baud Rate（波特率） | **9600** | 读卡模块默认 9600；本程序会在初始化时命令它切换到 115200 |
| Word Length（数据位） | **8 Bits** | 8 位数据 |
| Parity（校验位） | **None** | 无校验 |
| Stop Bits（停止位） | **1** | 1 位停止 |
| Data Direction | **Receive and Transmit（收发都开）** | 默认即可 |
| Over Sampling | **16 Samples** | 保持默认 |

### 6.3 打开串口中断（必须）

点击 **NVIC Settings（中断设置）** 标签：
- 找到 **USART1 global interrupt**（串口 1 全局中断）
- 勾选前面的 **Enabled（使能）** 复选框
- Preemption Priority（抢占优先级）填 **5**，Sub Priority（子优先级）填 **0**

> 解释：程序通过"串口接收中断"实时接收读卡模块发来的数据。不开启中断则读不到卡数据。

---

## 7. 配置串口 2（语音模块 USART2）

### 7.1 开启 USART2

1. 左侧外设树：
   ```
   Connectivity → USART2
   ```
2. Mode 下拉框选择 **Asynchronous**
3. 芯片图上 PA2、PA3 变成 USART2_TX、USART2_RX

### 7.2 设置参数

Parameter Settings 标签：

| 参数 | 填写 |
|---|---|
| Baud Rate | **9600** |
| Word Length | 8 Bits |
| Parity | None |
| Stop Bits | 1 |

### 7.3 打开串口中断

NVIC Settings 标签：
- 勾选 **USART2 global interrupt**
- Preemption Priority = **5**，Sub Priority = **0**

> 本工程实际主要用 USART2 的发送（给语音模块播报），接收中断开不开都能工作；按本表打开即可，无副作用。

---

## 8. 配置串口 3（数据输出 USART3）

> 【相对案例新增】案例原工程（源码 4.0）没有此串口，本项目新增用于实时输出读卡数据/电机状态/LED 状态。若你同时对照案例 `.ioc` 检查，找不到 USART3 是正常的。

### 8.1 开启 USART3

1. 左侧外设树：
   ```
   Connectivity → USART3
   ```
2. Mode 下拉框选择 **Asynchronous**
3. 芯片图上 **PB10** 变 USART3_TX、**PB11** 变 USART3_RX

> 引脚检查：如果 PB10/PB11 上出现了其他功能的图标（比如 I2C2），说明之前误配了，先删掉其他功能再选。本教程全流程不会占用这两个脚。

### 8.2 设置参数

| 参数 | 填写 | 说明 |
|---|---|---|
| Baud Rate | **115200** | 输出口用高速，上位机也要设成 115200 |
| Word Length | 8 Bits | |
| Parity | None | |
| Stop Bits | 1 | |

### 8.3 中断设置

NVIC Settings 标签：**不要勾选任何中断**。本串口只输出不接收，无需中断。

> 这个串口连电脑后，用串口助手（如 SSCOM、MobaXterm）以 115200 波特率查看实时数据。

---

## 9. 配置定时器 TIM2（电机 PWM）

### 9.1 开启两个 PWM 通道

1. 左侧外设树：
   ```
   Timers（定时器）→ TIM2
   ```
2. 右侧 "Mode" 区域有两个下拉框：
   - **Channel1（通道1）** 下拉框 → 选择 **PWM Generation CH1**（PWM 生成通道 1）
   - **Channel2（通道2）** 下拉框 → 选择 **PWM Generation CH2**（PWM 生成通道 2）
3. 芯片图上 PA0 变 TIM2_CH1、PA1 变 TIM2_CH2

> 注意：PA0 在芯片图上的名字可能显示为 TIM2_CH1_ETR，选 CH1 就行。

### 9.2 设置定时器参数

点击 Parameter Settings 标签：

| 参数 | 填写 | 说明 |
|---|---|---|
| Prescaler (PSC - 1)（预分频） | **71** | 72MHz ÷ 72 = 1MHz 计数频率 |
| Counter Mode（计数模式） | **Up（向上）** | 保持默认 |
| Counter Period (AutoReload Register - 1)（自动重装载值） | **999** | 1MHz ÷ 1000 = 1kHz PWM 频率 |
| Auto-reload preload | Disable | 保持默认 |

### 9.3 设置 PWM 参数 —— 关键！

在 Parameter Settings 页面往下滚动，找到 **PWM Generation Channel 1** 和 **PWM Generation Channel 2** 两组设置，**两组都**设置：

| 参数 | 填写 | 说明 |
|---|---|---|
| Mode | **PWM Mode 2（PWM 模式 2）** | ★ 必须选 Mode 2！控制电机极性的关键 |
| Pulse（脉冲/初始占空比） | **0** | 初始速度 0 |
| Output compare preload | **Disable** | 保持默认（案例即为 Disable） |
| Fast Mode | **Enable** | 按案例设置（CubeMX 默认是 Disable，需手动改为 Enable） |
| CH Polarity（极性） | **High** | 保持默认 |

> 为什么必须 PWM Mode 2：
> 程序按"速度 0~999 对应平均电压"的方式控制电机。Mode 2 下 CCR 值越大输出高电平时间越长，配合程序里的互补双路输出实现电机转速控制。如果误选 Mode 1，电机方向会反或者不动。

---

## 10. 配置 ADC1（电位器）

### 10.1 添加 ADC 通道

1. 左侧外设树：
   ```
   Analog（模拟）→ ADC1
   ```
2. 右侧 "Mode" 区域，**IN9** 这一行（Channel 列表）：
   - 点击下拉框 → 选择 **IN9**（通道 9）
   - 芯片图上 PB1 变 ADC1_IN9
3. 如果弹窗问 "ADC Out Of Reset... 是否使能 ADC 时钟"，选 Yes

### 10.2 设置 ADC 参数

Parameter Settings 标签：

| 参数 | 填写 | 说明 |
|---|---|---|
| Continuous Conversion Mode（连续转换模式） | **Enabled（使能）** | 持续采样 |
| Discontinuous Conversion Mode | Disabled | 默认 |
| Scan Conversion Mode | Disabled | 单通道不需要扫描 |
| External Trigger Conversion Source | **Regular Conversion launched by software（软件触发）** | 保持默认 |
| Data Alignment（数据对齐） | **Right alignment（右对齐）** | 默认 |
| Number Of Conversion（转换数量） | **1** | 单通道 |

在 **Rank 1**（转换序列 1）区域：
- Channel：自动为 IN9
- Sampling Time（采样时间）：**28.5 Cycles（28.5 周期）**

---

## 11. 为什么本版本不需要 FreeRTOS

### 11.1 本版本没有操作系统

- 左侧外设树中的 **Middleware and Software Packs（中间件）** 部分，本版本**什么都不用配置**（FreeRTOS 的选项保持空白/Disabled 即可）。
- 生成工程后，目录里**不会有** `Middlewares/` 文件夹，也不会生成 `freertos.c`。这是正常的。

### 11.2 功能能不能实现？（裸机可行性）

本项目所有功能都是"顺序执行 + 短时间轮询"性质，完全可以裸机实现：

| 原 RTOS 任务 | 裸机对应 | 可行性 |
|---|---|---|
| RFID_Task（读卡/播报/LED） | `RFID_Process()` 状态机函数 | ✅ 每次调用处理一个状态就返回 |
| Motor_Control_Task（电机时序） | `Motor_Process()` 状态机函数 | ✅ 用 `HAL_GetTick()` 毫秒计数 |
| 串口接收中断 | 完全相同（中断不依赖操作系统） | ✅ |
| 播报后 800ms 等待 | 状态机内"记录开始时刻→到点再继续"，非阻塞 | ✅ |

主循环每圈都调用两个函数：
```c
while (1)
{
    RFID_Process();   // 读卡、播报、LED、去重、触发（非阻塞）
    Motor_Process();  // 电机晚启动/缓启动/降速/停车/重启（非阻塞）
}
```

> 唯一的"代价"：语音播报时串口发送是阻塞的（约 2~17ms，单块 16 字节 @9600），这段时间两个状态机都会停顿。电机缓启动是按 10ms 一档更新的，一次停顿最多造成"跳过一两档"，启动段有轻微顿挫但完全可接受。

### 11.3 裸机版的优势

- 代码量小、结构简单，适合学习与比赛现场快速修改
- 不占用额外内存（若用 RTOS 版，FreeRTOS 堆 8192B + 内核/空闲任务开销约 8~9KB；裸机版全部省去）
- 调试直观（单步执行逻辑线清晰）

---

## 12. 配置 NVIC 中断优先级

### 12.1 打开 NVIC

左侧外设树：
```
System Core → NVIC
```

### 12.2 设置优先级分组

在页面中间找到 **Priority Group（优先级分组）** 设置：
- 下拉框选择 **4 bits preemption priority, 0 bits subpriority**（4 位抢占优先级，0 位子优先级）

> 解释：优先级组 4 = 抢占优先级 4 位（0~15），没有子优先级。与 RTOS 版保持一致（方便两版代码互转）。

### 12.3 核对中断列表

在 NVIC 中断列表（Enable 列勾选框）中确认：
- [x] USART1 global interrupt —— Priority 5（前面已设）
- [x] USART2 global interrupt —— Priority 5
- [x] 三个 Fault 中断（Hard fault 等）保持使能（默认）
- 其他保持默认

> 【裸机版差异】RTOS 版会有一个 TIM1 update interrupt（时基用），本版本**没有**——因为时基用的是 SysTick。

---

## 13. 工程设置（Project Manager）

点击顶部标签：
```
Project Manager（工程管理）
```

### 13.1 基本信息

| 字段 | 填写 | 说明 |
|---|---|---|
| Project Name（工程名） | `rfid_tts_baremetal` | 建议用这个 |
| Project Location（保存位置） | 选择一个目录，如 `D:\stm32\` 或本机任意路径 | 建议保存到仓库外的独立目录，生成后再对照/合并代码 |
| Toolchain / IDE（工具链） | **CMake**（推荐，后续可到 Linux 本机编译验证）；也可选 **MDK-ARM** 或 **STM32CubeIDE** | 按你有的编译器选。选 CMake 时 CubeMX 会生成 CMakeLists.txt（内含 Windows 工具链路径，拷到 Linux 后按 §16.3 调整） |
| Minimum Heap Size（最小堆大小） | `0x400`（1024） | 保持默认 |
| Minimum Stack Size（最小栈大小） | `0x800`（2048） | 保持默认 |

### 13.2 代码生成选项

点左侧 **Code Generator（代码生成器）** 标签：

- 勾选 **Generate peripheral initialization as a pair of '.c/.h' files per peripheral**（每个外设生成独立的 .c/.h 文件）
  > 不勾的话所有外设代码都塞在一个 main.c 里，很难维护。**务必勾选**。
- 勾选 **Copy only the necessary library files**（只拷贝需要的库文件）
- 勾选 **Add necessary files as source**（需要的文件作为源码添加）
- 其余保持默认

### 13.3 用户代码区设置

左侧 **Advanced Settings（高级设置）** 标签保持默认即可。

---

## 14. 生成代码

1. 点击软件右上角的按钮：
   ```
   GENERATE CODE（生成代码）
   ```
   （界面右上角，一个像"锤子+代码"的图标）
2. 如果弹出 "Open Project?"（是否打开工程）提示，选 **Yes** 会用对应 IDE 打开，选 No 也行
3. 生成完成后，在保存目录下会看到：

```
rfid_tts_baremetal/
├── Core/
│   ├── Inc/            # 头文件（main.h、usart.h、tim.h...）
│   └── Src/            # 源文件（main.c、usart.c、tim.c、adc.c、gpio.c、
│                       #   stm32f1xx_it.c、stm32f1xx_hal_msp.c）
├── Drivers/            # STM32 HAL 库
├── MDK-ARM/            # 如果选了 MDK-ARM 工具链，这里是 Keil 工程
└── rfid_tts_baremetal.ioc   # CubeMX 工程文件（以后双击它就能重新打开配置）
```

> 注意：**没有** `Middlewares/` 和 `freertos.c` —— 这是裸机版的正常现象。

---

## 15. 生成后必须手动补充的代码

CubeMX 生成的代码只是"硬件驱动框架"，还需要补业务代码。以下各点对应本项目的 `02-项目说明.md` 代码结构说明。

### 15.1 加入项目自写代码目录

把本项目配套交付的以下目录/文件复制进生成的工程根目录：
- `config.h`（全部可调参数，含触发词规则表）
- `hardware/`（USART、rfid_card、pwm、LED、ADC、DEBUG 驱动）
- `Task/`（状态机实现：rfid_process.c/h、motor_process.c/h）

> 说明：`config.h`、`hardware/`、`Task/` 属于业务代码，随项目交付物提供（见 `stm32/README.md` 第 6 节状态说明）；`Core/`、`Drivers/` 由 CubeMX 生成，不要覆盖。

然后把它们加入编译：
- **Keil 用户**：在 MDK-ARM 工程的 Groups 里 Add Existing Files，把 hardware/ 和 Task/ 下的 .c 文件加进去，并在 C/C++ Include Paths 里加 `config.h`、`hardware` 各子目录、`Task` 目录的路径
- **CMake/Makefile/IDE 用户**：让编译配置包含这些目录（CMake 用户把 `hardware/`、`Task/` 加入源文件列表即可，详见交付物 CMakeLists 示例）

### 15.2 修改 usart.c：读卡模块波特率切换（必做）

打开 `Core/Src/usart.c`，找到：

```c
/* USER CODE BEGIN USART1_Init 2 */
/* USER CODE END USART1_Init 2 */
```

在中间的注释区填入：

```c
/* USER CODE BEGIN USART1_Init 2 */
  SetBound115200();                                          // 命令读卡模块切换到115200
  while( !__HAL_UART_GET_FLAG( &huart1, UART_FLAG_TC ) );    // 等待发送完成
  huart1.Init.BaudRate = 115200;                             // 本机串口同步改为115200
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
/* USER CODE END USART1_Init 2 */
```

> 解释：读卡模块 U13T 默认 9600。程序上电时先按 9600 发一条"设波特率"命令（函数 SetBound115200 在 hardware/rfid_card/Card.c 里），模块切换后（可能记忆 115200 也可能不记忆，无关紧要），MCU 自己也无条件切到 115200，双方最终必然匹配。不这样做读卡永远没反应。切换流程幂等，无需人工干预。

### 15.3 修改 main.c：初始化与主循环（核心）

**第 1 步：先补头文件与外部变量声明（否则照抄下面的代码会编译报"未声明"错误）**

打开 `Core/Inc/main.h`，找到：

```c
/* USER CODE BEGIN Includes */
/* USER CODE END Includes */
```

填入：

```c
/* USER CODE BEGIN Includes */
#include "BSP_USART.h"
#include "led.h"
#include "PWM.h"
#include "Card.h"
#include "Debug.h"
/* USER CODE END Includes */
```

再打开 `Core/Src/main.c`，找到文件顶部的：

```c
/* USER CODE BEGIN 0 */
/* USER CODE END 0 */
```

填入（读卡接收缓冲变量定义在 Card.c 中，需在此声明为外部变量）：

```c
/* USER CODE BEGIN 0 */
extern uint8_t card_res;
/* USER CODE END 0 */
```

**第 2 步：填入初始化代码**

打开 `Core/Src/main.c`，找到：

```c
/* USER CODE BEGIN 2 */
/* USER CODE END 2 */
```

填入：

```c
/* USER CODE BEGIN 2 */
  LED_Sta( 0 );                 // LED 初始化熄灭
  PWM_Init();                   // 启动 PWM
  Motor_Control( 0 );           // 电机初始速度 0
  Dbg_Init();                   // 数据输出串口初始化（可选）
  HAL_UART_Receive_IT( &CARD_HAL_USARTx, (uint8_t *)&card_res, 1 ); // 开读卡串口接收中断
/* USER CODE END 2 */
```

再找到文件末尾的：

```c
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
```

在 `/* USER CODE BEGIN WHILE */` 注释行**下方**、`while (1)` 的循环体内（即 `/* USER CODE END WHILE */` 之前）填入两行调用（**不要**重复写 `while (1) {` 包裹，CubeMX 模板已经带好了）：

```c
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    RFID_Process();    // 读卡、播报、LED、去重、触发（非阻塞状态机）
    Motor_Process();   // 电机时序（非阻塞状态机）
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
```

> 【裸机版差异】RTOS 版此时已由调度器接管（osKernelStart），裸机版则由这个 while(1) 主循环驱动一切。

### 15.4 printf 重定向到语音串口（必做）

工程需要把 C 库的 printf 输出重定向到 **USART2**（TTS 语音模块）。

- **MDK-ARM（Keil）**：在 hardware/USART/BSP_USART.c 中已实现：
  ```c
  int fputc(int ch, FILE *fp) { HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, 0xffff); return ch; }
  ```
  Keil 工程里勾选 "MicroLIB" 即可支持 printf（MDK Options → Target → Use MicroLIB）。
- **GCC（STM32CubeIDE/CMake/本机）**：需要提供 `_write` 重定向。交付物在 hardware/USART 提供 GCC 版 syscall 文件，注意两点：
  - 目标必须是 **huart2**（案例原版 syscall.c 重定向到 huart1 是给读卡调试用的，照抄会语音走错串口）
  - 若手改案例 syscall.c：`_write` 里的 **`USART1->SR` 也要一并改成 `USART2->SR`**（寄存器名与句柄都要改，只改一个会变成"等 USART1 状态、发 USART2"的隐性错误）；`_read` 同样处理
  - 该 syscall 文件仅在 GCC 工具链下编译（文件内已用 `__GNUC__` 条件编译包裹；ARM Compiler 5 不支持 `__has_include`，Keil 工程请勿加入此文件）

### 15.5 修改 stm32f1xx_it.c：HardFault 自动重启

案例源码本身已带此功能（`HardFault_Handler` 内 `HAL_NVIC_SystemReset()`），CubeMX 新生成工程默认没有，需要手动补上。

打开 `Core/Src/stm32f1xx_it.c`，找到 `HardFault_Handler`：

```c
void HardFault_Handler(void)
{
  while (1)
  {
    /* USER CODE BEGIN W1_HardFault_IRQn 0 */
    HAL_NVIC_SystemReset();     // 程序卡死自动重启（案例已带，CubeMX 新工程需手动补）
    /* USER CODE END W1_HardFault_IRQn 0 */
  }
}
```

### 15.6 确认读卡中断回调存在

`HAL_UART_RxCpltCallback`（串口接收完成回调）已在 hardware/rfid_card/Card.c 中实现（解析读卡模块协议），**不要**在别处重复定义，否则编译报重复定义错误。

---

## 16. 编译与烧录

### 16.1 方法 A：Keil MDK（Windows）

1. 打开生成的 `MDK-ARM/rfid_tts_baremetal.uvprojx`
2. 编译：点击工具栏 "Build"（图标像字母 F7，或按 F7）
3. 烧录：连接 ST-Link 到电脑和板子（SWDIO→PA13、SWCLK→PA14、GND→GND、3.3V→3.3V），点 "Download"（LOAD 图标）
4. 成功烧录后按板子复位键，观察现象

### 16.2 方法 B：STM32CubeIDE（Windows/Linux）

1. CubeMX 生成时工具链选 STM32CubeIDE，生成后点 "Open Project" 直接打开
2. 点工具栏的锤子（Build），再点绿色虫子图标旁的下载箭头（Run/Debug）
3. 首次会提示选择调试器，选 ST-LINK

### 16.3 方法 C：Linux 本机命令行（本项目配套）

> ⚠️ 工具链路径问题（必看）：CubeMX 在 Windows 上生成 CMake 工程时，`CMakeLists.txt` 里会**硬编码 Windows 上 STM32CubeIDE 的 arm-none-eabi-gcc 绝对路径**（形如 `C:/ST/STM32CubeIDE_*/.../bin/arm-none-eabi-gcc`）。把工程拷到 Linux 后直接 cmake 会报"找不到编译器"。解决办法（二选一）：
> ① 删除/注释 CMakeLists 中 `set(CMAKE_C_COMPILER ...)` 与 `set(CMAKE_CXX_COMPILER ...)` 两行，让 CMake 使用系统 PATH 里的 `arm-none-eabi-gcc`（本机已装 14.2.1）；
> ② 或把其中 Windows 路径改为本机工具链路径。
> 另外，CubeMX 的 Makefile 工具链生成的是 Makefile（非 CMakeLists.txt），Linux 编译命令对应为 `make`（同样需先改 `CROSS_COMPILE` 路径）。本项目文档按 CMake 工具链编写。

```bash
# 在项目根目录（含 CMakeLists.txt）
cmake -B build
cmake --build build -j
# 生成 build/rfid_tts_baremetal.bin

# 烧录（ST-Link 已连接）
st-flash write build/rfid_tts_baremetal.bin 0x08000000
```

### 16.4 查看数据输出串口

USB 转 TTL 模块接 USART3（PB10=TX）与电脑连接，串口工具设置 **115200，8，N，1**：
```bash
# Linux 本机示例
screen /dev/ttyUSB0 115200
```
应看到类似输出：
```
[SYS] boot   LED=OFF MOTOR=STOP
[RFID] 4: CC AB D1 F4
[LED] ON
[MOTOR] RAMPUP speed=345
```

---

## 17. 常见问题 FAQ

| 现象 | 原因与解决 |
|---|---|
| 时钟树有红字，显示超频 | PLL 或分频设错。按第 4.2 表逐项核对（HSE 8M、×9、72M） |
| 编译报错：找不到 `stm32f1xx_hal.h` | 头文件路径没加全：`Drivers/STM32F1xx_HAL_Driver/Inc`、`Drivers/CMSIS/Device/ST/STM32F1xx/Include`、`Drivers/CMSIS/Include` |
| 编译报错：重复定义 `HAL_UART_RxCpltCallback` | 你自己又在别处写了一遍这个回调，删掉重复的 |
| 烧录时提示 "No target connected" | SWD 没配（第 3.2 步）、接线错、板子没供电 |
| 电机不转 | ① TIM2 是不是 PWM Mode 2；② `MOTOR_TARGET_SPEED` 是否 0；③ 电机驱动板供电 |
| 电机缓启动不平滑（一顿一顿） | 属正常现象（串口发送期间有毫秒级停顿，单块 16 字节约 2~17ms），可减小 `MOTOR_SLOW_RAMP_STEP_MS`（如 5）让阶梯更细，或接受现状（不影响功能） |
| 读卡没反应 | ① 波特率切换代码（15.2）漏了；② 读卡模块没共地；③ 卡片不在感应区 |
| 语音不播报 | printf 重定向是否到 huart2（15.4）；TTS 模块 5V 供电；波特率 9600 |
| 数据串口乱码 | 上位机波特率要 **115200**；接线 TX/RX 反了 |
| 两个状态机"卡死"互不影响吗？ | 两函数均非阻塞（每次调用即返回），最坏阻塞点是语音串口发送 2~17ms，功能不受影响 |

---

## 附录：本教程全部配置速查表

（以下为生成前的最终配置汇总，用于快速复查）

**芯片**：STM32F103C8Tx（LQFP48）

**时钟**：HSE 8MHz × PLL9 = 72MHz；AHB /1；APB1 /2（36M）；APB2 /1；ADC /6（12M）

**SYS**：Debug = Serial Wire；Timebase = **SysTick**（裸机版，保持默认）

**引脚与功能**：

| 引脚 | 功能 | 关键参数 |
|---|---|---|
| PA0 | TIM2_CH1 PWM | PSC 71、ARR 999、PWM Mode 2、Pulse 0 |
| PA1 | TIM2_CH2 PWM | 同上 |
| PA2 | USART2_TX | 9600 8N1 |
| PA3 | USART2_RX | 9600 8N1 |
| PA8 | GPIO_Output | 上拉、初始 High、速度 High |
| PA9 | USART1_TX | 9600 8N1（程序内切 115200） |
| PA10 | USART1_RX | 同上 |
| PA13 | SWDIO | Serial Wire |
| PA14 | SWCLK | Serial Wire |
| PB1 | ADC1_IN9 | 28.5 cycles、连续转换 |
| PB10 | USART3_TX | 115200 8N1，无中断 |
| PB11 | USART3_RX | 同上 |
| PB12 | GPIO_Output | 上拉、初始 High |
| PC13 | GPIO_Output | 上拉、初始 High |
| PD0/PD1 | OSC_IN/OUT | 8MHz 晶振 |

**中断（NVIC，优先级组 4）**：USART1=5、USART2=5（无 TIM1 中断）

**Middleware**：无（不使用 FreeRTOS）

**Project Manager**：每外设独立 .c/.h；只拷贝必要库文件

---

### 与 RTOS 版教程差异小结

| 章节 | RTOS 版 | 裸机版（本教程） |
|---|---|---|
| 3.3 时基 | Timebase = **TIM1** | Timebase = **SysTick**（默认） |
| 11 中间件 | 配置 FreeRTOS（CMSIS_V1、两任务、Heap） | **不配置**，无 Middleware |
| 12 NVIC | 有 TIM1_UP 中断 | 无 TIM1 中断 |
| 14 生成物 | 有 Middlewares/、freertos.c | 无 |
| 15.3 main | 初始化 + 调度器启动（任务由 freertos.c 创建） | 初始化 + while(1) 主循环调两个状态机 |
