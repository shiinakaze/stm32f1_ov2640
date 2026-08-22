# stm32f1_ov2640 — LLM 项目简报

本文件面向后续接手本项目的 LLM/自动化代理，提供无对话上下文也能安全工作的全部事实。
人读的教程在 `docs/app-manual.md`。

## 1. 项目定位与当前状态

- **做什么**：STM32F103C8（Blue Pill）+ OV2640 → MJPEG 采集 → 两种输出通道（编译期宏）：
  UVC USB 摄像头（默认，Windows 相机实测可用）/ UART 1.5Mbaud 调试通道。
- **状态**：描述符声明 320x240、160x120、640x360 三档，**全部固定 30fps 单档**
  （Windows 相机对 MJPEG 源默认按 30fps 协商，只声明 15fps 实测不出画面；
  声明帧率只是主机渲染节奏，实际出帧受流水线限制：约 10~13fps（320x240）/
  15~18fps（160x120）/ 6~8fps（640x360，发送带宽 184B/ms 为瓶颈））。
- **诊断手段**：OLED 第 1/2 行显示 PID/MID（初始化通过的标志），第 3/4 行
  实时显示 F=已送出帧数 / E=丢弃帧数——F 增长而画面黑 → 主机侧问题；
  E 增长 → 采集侧问题。初始化失败会显示错误码并停机（1=SCCB 无应答，
  2=PID 不符，3=IMAGE_MODE 回读校验失败）。
- **Git**：main 分支；仓库内 `middleware/usb/Projects/STM32F407-UVC/` 是 untracked 的
  F4 参考工程（红龙 407 板），只可参考其描述符结构与 OV2640 窗口寄存器写法，
  **端点驱动模型与 F1 完全不同，勿照抄**。

## 2. 构建环境

- Keil MDK，ARMCC V5.06（AC5），`-O1`，工程 `stm32f1_ov2640.uvprojx`，目标 STM32F103C8。
- 命令行构建：
  ```
  "C:/Users/shiinakaze/AppData/Local/Keil_v5/UV4/UV4.exe" -b stm32f1_ov2640.uvprojx -j0 -o build.log
  ```
  （`-b` 增量，`-r` 全量；退出码 0 = 成功。）
- 传输通道切换：`app/main.h` 的 `APP_TRANSPORT`（`APP_TRANSPORT_UVC` / `APP_TRANSPORT_UART`）。
- 两种配置的占用（armcc -O1，2026-08 实测）：

  | 配置 | Flash | RAM（/20480B） |
  |---|---|---|
  | UVC | Code 15596 + RO 1372 | RW 1888 + ZI 17672 = **19560**（裕量约 920B，含 16KB 帧缓冲） |
  | UART | Code 17564 + RO 1372 | RW 1896 + ZI 17648 = **19544**（裕量约 940B，双 8KB 缓冲） |

  两种模式都会编译 USB 栈（UART 模式不初始化、外设保持复位，只多占 Flash）。

## 3. 代码地图

```
app/main.c               主循环：UVC 串行流水线 / UART 双缓冲流水线（#if 分支）；
                         UVC 帧缓冲 16KB；OLED F/E 帧计数诊断；init 失败报错停机
app/main.h               APP_TRANSPORT 宏
app/stm32f10x_it.c       USB_LP_CAN1_RX0_IRQHandler → USB_Istr()
drivers/ov2640.c/.h      传感器驱动：ov2640_init() 返回 ov2640_err_t
                         （PIDH=0x26 校验 + IMAGE_MODE 回读哨兵）；
                         内部 bank 缓存的 write_reg/read_reg（自动写 0xFF 组切换）；
                         capture 拆分为 帧同步/位扫描读帧/JPEG 标记定位 三段，
                         所有等待带 ~2s 超时（拔摄像头不再挂死）；
                         OV2640_DVP_SPEED_DIV 宏控制 PCLK（当前 64 → 750kHz）；
                         set_output_size 按输出宽高比裁剪 DSP 输入窗口（640x360
                         16:9 → 在 640x480 窗口内垂直居中裁剪，避免画面压扁）
drivers/ov2640_config.c/.h  寄存器表：svga_rgb565_25fps_cfg（177 项，与 OV2640
                         Software Application Notes 1.03 §13.2 逐项一致）+
                         ov2640_jpeg_mode_cfg；extern 用 _len 常量声明长度；
                         寄存器名宏对应 DS V1.6 表 12/13
drivers/sccb.c, i2c.c    软件 SCCB（write/read_reg 返回状态码，读为出参式）；
                         i2c.c 里有 SWJ_JTAGDisable（PB3/PB4 可用前提）
drivers/uart.c           UART1 TX DMA（DMA1_CH4，无中断，1.5Mbaud）
middleware/delay.c       SysTick 延时，MCU_SYSCLK=72（曾误写 128 导致延时偏慢 1.78 倍）
middleware/usb/src|inc   ST USB-FS-Device 库 V4.1.0（勿改）
middleware/usb/uvc/      UVC 应用层（本项目核心增量）：
  usb_conf.h             PMA 地址、EP_NUM、IMR_MSK、EP 回调映射（EP1_IN_Callback→uvc_ep1_in_callback）
  usb_desc.c/.h          全部描述符（配置总长 211，VC 部 39，VS 部 121；
                         3 个 30B 帧描述符，全部单档 333333 = 30fps）
  usb_prop.c/.h          DEVICE_PROP/Device_Table/User_Standard_Requests、probe/commit、EP 初始化
  usb_pwr.c/.h           PowerOn/Suspend/Resume（简化版，不做时钟门控）、DEVICE_STATE 枚举
  usb_istr.c/.h          USB_Istr 事件分发（CTR/RESET/SUSP/WKUP/ESOF；SOF 未启用）
  usb_bsp.c/.h           USB 时钟/NVIC、usb_irq_lock/unlock（NVIC 关/开 USB 中断）、U_ID 序列号
  usbd_uvc.c/.h          ★ 流式状态机：load_next_packet（CTR 驱动装包）、静默模式、
                         commit 解析（res_table 帧索引→分辨率查表，非法索引回退 320x240）
docs/app-manual.md       人读应用手册
docs/plan.md             本文件
README.md / README.zh-CN.md  英文/简体中文说明
```

## 4. 硬件事实（RM0008 Rev 21 + OV2640 DS V1.6 查证，违反即坏）

以下来自 RM0008 §23.4.4 / Table 171 与实测：

1. **F1 USB 的 ISO 端点强制双缓冲**：每次 IN 传输后硬件自动翻转 DTOG_TX，
   交替使用 BUF0（TX 描述符槽）与 BUF1（RX 描述符槽）。不存在单缓冲 ISO。
   只初始化一组描述符 ⇒ 主机只收到第 1 个包，其余全是垃圾 ⇒ 枚举正常但相机黑屏。
2. **ISO 端点 STAT 只能写 DISABLED(00) 或 VALID(11)**；传输完成后硬件保持 VALID。
   **对 ISO 写 NAK/STALL 是未定义行为**（RM 原文）。禁止 bulk 式 NAK→VALID 重装。
3. 每次传输结束 CTR_TX 置位，且此时 DTOG_TX 已指向"硬件下一个要用的缓冲"。
   软件永远读 DTOG_TX 决定装哪个缓冲，不要自己维护缓冲序号。
4. IN 方向的缓冲计数是裸值直写：`SetEPDblBuf0Count(ep, EP_DBUF_IN, n)` 写 COUNT_TX，
   `SetEPDblBuf1Count(ep, EP_DBUF_IN, n)` 写 COUNT_RX。不要用 `SetEPRxCount`（块编码）。
5. PMA 共 512B；BTABLE 占 0x00-0x3F，但只用 EP0/EP1 时 0x10-0x3F 是死空间可复用。
   当前布局：EP0 RX@0x10(64) EP0 TX@0x50(64) BUF0@0x90(184) BUF1@0x148(184)，正好用满。
6. USB 必须满足 USBCLK=48MHz ⇒ 系统锁死 72MHz（PLL/1.5）；禁止超频路径。
7. OV2640 位扫描采集期间任何中断都会丢字节毁帧 ⇒ 采集必须在 NVIC 层锁 USB 中断
   （`usb_irq_lock/unlock`），锁之前须保证两个缓冲已装成空包（静默模式）。
8. **GPIO 位扫描采集的 PCLK 上限约 900kHz**：采集热循环（72MHz/-O1）每字节
   CPU 耗时约 700~840ns（实测反汇编：两次 PCLK 沿轮询 + 读存储 + HREF 检查），
   字节周期需 ≥1.5 倍裕量。当前 R_DVP_SP 分频 64 → PCLK 750kHz（裕量约 1.6 倍）；
   分频 32（1.5MHz）实测全丢帧黑屏。调参只改 `OV2640_DVP_SPEED_DIV`。
9. Windows 相机对 MJPEG 源默认按 30fps 协商；描述符只声明 15fps 时不出画面
   （VLC/ffmpeg 不受影响）。帧描述符统一 30fps 单档。
10. Windows 缓存 USB 描述符：改描述符后需拔插 USB 或设备管理器卸载重枚举。

## 5. 设计不变量（改代码前自查）

- ISO 数据路径只允许出现在 `usbd_uvc.c`，且只由 CTR_TX（`uvc_ep1_in_callback`）驱动；
  不存在 SOF 驱动路径（IMR_MSK 故意不含 CNTR_SOFM）。
- 帧结束语义：`frame_done_flag` 在 **EOF 包传输完成后的那次 CTR** 置位（不是装入时），
  同时把双缓冲装成空包；main 等 `uvc_frame_done()` 才允许锁中断采集。
- `uvc_feed_frame` 写序固定：`next_frame` → `frame_done_flag=0` → `next_len`（先指针后长度，
  与 CTR 无锁竞争的关键）。
- FID 在装 EOF 包时翻转；空包与新帧首包共用翻转后的 FID（主机视角每帧单次翻转）。
- 分辨率切换由 commit 触发（`res_pending`），但 SCCB 写入只允许在主循环采集前执行
  （`uvc_poll_resolution` → `ov2640_set_output_size`），禁止在 USB 中断里做 I2C。
  `ov2640_set_output_size` 同时按输出宽高比重写 DSP 输入窗口（HSIZE1/VSIZE1/XOFFL/YOFFL，
  单位 real/4）——4:3 输出必须还原为完整 640x480 窗口（0xA0/0x78/0/0）。
- EP1 寄存器操作只发生在 USB 中断上下文（enable/disable/reset 也都源自该上下文），无并发。
- 帧长上限 16KB（单缓冲容量；描述符 640x360 帧声明 dwMaxVideoFrameBufferSize=16384，
  其余档 12288），超限帧在采集层丢弃（截断无 EOI）。
- `ov2640_init` 校验链：SCCB 读 PID（PIDH=0x26，重试 3 次）→ 下载两份配置表 →
  R_DVP_SP 手动分频 → 回读 IMAGE_MODE==0x10；任一失败 OLED 报错误码停机，
  不进入 USB 初始化（无传感器时设备不应枚举）。
- UVC/UART 两模式 RAM 裕量均仅 ~920B：加全局变量前先算总量（栈 1KB 已含在 ZI 内）。
- 采集热路径（`ov2640_read_frame`）改代码前先反汇编数每字节周期，保持 PCLK 裕量 ≥1.5 倍。

## 6. 已修复的关键缺陷（教训存档）

1. **黑屏 v1（已修复）**：ISO 按单缓冲实现 + 每包写 NAK → 违反上述硬件事实 1/2。
   症状：设备管理器正常、Windows 相机黑屏。修复 = 双缓冲 + DTOG 驱动 + STAT 恒 VALID。
2. SOF 中断装包存在"赶不上主机同帧 IN 轮询"的帧内丢包风险 → 改为 CTR_TX 驱动
   （发完立即装下一包，提前近 1ms），SOF 从中断屏蔽中移除。
3. 双缓冲帧缓冲（UART 时代）在 UVC 串行流水线下无意义，改单缓冲，腾出 RAM
   （现为 16KB，配合 640x360）。
4. `usb_lib.h` 需要应用层提供 `hw_config.h`（垫片只含 `#include "main.h"`）。
5. 类请求码（GET_CUR 0x81 等）不在库头文件里，需在 `usb_prop.h` 自行定义。
6. **黑屏 v2（已修复）**：R_DVP_SP 从 0x64（480kHz）提速到 0x20（1.5MHz）后
   采集轮询跟不上（每字节 CPU 700~840ns > 667ns 字节周期）→ 全帧丢弃、主机只收
   空包、Windows 相机黑屏。修复 = 分频 64（750kHz）+ OLED F/E 帧计数诊断。
   教训：采集提速前先反汇编数指令周期。
7. `middleware/delay.c` 的 MCU_SYSCLK 误写 128（超频时代残留），而系统实为 72MHz，
   所有延时偏慢 1.78 倍（SCCB 时钟偏慢无害但失真）。已改 72。
8. Windows 相机 15fps-only 不出画面 → 帧描述符全部改 30fps 单档
   （15fps 档曾被短暂引入后又移除）。

## 7. 可选优化方向（未做，按价值排序）

1. **采集热循环优化**：VSYNC/HREF/PCLK 同在 GPIOB，每字节可用单次 IDR 读同时取
   三信号（当前分别读三次），理论上可把每字节 CPU 耗时压到 ~400ns，
   PCLK 即可安全提到 960kHz~1.5MHz，采集时间再减半。改前先反汇编验证。
2. **带宽提升**：BUF0/BUF1 指向同一 PMA 地址 + 保留各自计数，理论上可回到 368B/包
   （需实测硬件是否接受同地址双缓冲；收益：320x240 提升到约 12~15fps）。
3. EP0 缓冲压缩（RX/TX 各 32B）可再给 ISO 腾 64B。
4. 帧率上限：当前 SVGA 25fps 基准（FLL=0x22）；若换 30fps 基准（FLL=0x00 +
   60Hz 频闪参数，App Notes §3.1/§5.3），采集等待期可缩短，需重调频闪组。
5. 640x360 丢帧观察：亮场景帧可能超 16KB 被丢（E 计数可见），可按分辨率调 Qs（0x44）
   量化系数换更小帧。
6. UVC 1.1 + dwClockFrequency/PTS 时间戳（主机端更平滑）。

## 8. 验证清单（改动后执行）

1. `UV4 -r` 全量构建 UVC 默认配置：0 错 0 警；记录 Program Size 对比 RAM 裕量（≤19560 附近）。
2. 切 `APP_TRANSPORT_UART` 构建，RAM ≤ 19544 附近；改回 UVC 再构建。
3. 烧录（J-Link），拔插 USB（描述符缓存），设备管理器出现"摄像头"。
4. OLED 检查：第 1/2 行 PID(0x2641)/MID(0x7FA2) → 初始化通过；开流后第 3 行 F 计数增长。
5. Windows 相机出图；用 ffplay 强制各分辨率验证：
   `ffplay -f dshow -framerate 30 -video_size 640x360 -i video="STM32F1 OV2640 UVC"`
   （640x360 画面比例应正常不压扁）。
6. 若黑屏：先看 OLED——E 增长 → 采集侧（PCLK 分频回调 100 复测）；
   F 增长 → 主机侧（`ffmpeg -list_options true -f dshow -i video=...` 看协商结果，
   USBPcap 抓 EP 0x81 确认包流）；再按 `docs/app-manual.md` 第 10 节表格排查。
