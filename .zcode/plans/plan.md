# stm32f1_ov2640 — LLM 项目简报

本文件面向后续接手本项目的 LLM/自动化代理，提供无对话上下文也能安全工作的全部事实。
人读的教程在 `docs/app-manual.md`。

## 1. 项目定位与当前状态

- **做什么**：STM32F103C8（Blue Pill）+ OV2640 → MJPEG 采集 → 两种输出通道（编译期宏）：
  UVC USB 摄像头（默认，已验证 Windows 相机应用正常出图）/ UART 1.5Mbaud 调试通道。
- **状态**：UVC 模式在 Windows 实测可用；描述符声明 320x240 与 160x120、各 30/15fps；
  实际出帧约 10~13fps（320x240）/ 15~18fps（160x120），受采集-发送串行流水线限制。
- **Git**：main 分支；仓库内 `middleware/usb/Projects/STM32F407-UVC/` 是 untracked 的
  F4 参考工程（红龙 407 板），只可参考其描述符结构，**端点驱动模型与 F1 完全不同，勿照抄**。

## 2. 构建环境

- Keil MDK，ARMCC V5.06（AC5），`-O1`，工程 `stm32f1_ov2640.uvprojx`，目标 STM32F103C8。
- 命令行构建：
  ```
  "C:/Users/shiinakaze/AppData/Local/Keil_v5/UV4/UV4.exe" -b stm32f1_ov2640.uvprojx -j0 -o build.log
  ```
  （`-b` 增量，`-r` 全量；退出码 0 = 成功。）
- 传输通道切换：`app/main.h` 的 `APP_TRANSPORT`（`APP_TRANSPORT_UVC` / `APP_TRANSPORT_UART`）。
- 两种配置的占用（armcc -O1）：

  | 配置 | Flash | RAM（/20480B） |
  |---|---|---|
  | UVC | Code 15088 + RO 1324 | RW 1884 + ZI 13580 = **15464** |
  | UART | Code 17120 + RO 1332 | RW 1892 + ZI 17644 = **19536**（裕量约 540B，加全局变量前先算） |

  两种模式都会编译 USB 栈（UART 模式不初始化、外设保持复位，只多占 Flash）。

## 3. 代码地图

```
app/main.c               主循环：UVC 串行流水线 / UART 双缓冲流水线（#if 分支）
app/main.h               APP_TRANSPORT 宏
app/stm32f10x_it.c       USB_LP_CAN1_RX0_IRQHandler → USB_Istr()
drivers/ov2640.c/.h      传感器驱动；单缓冲 API：set_frame_buffer(buf,size)/capture/get_ready_frame
                         capture 为 GPIO 位扫描（VSYNC/HREF/PCLK 轮询读 PA0-7）
drivers/ov2640_config.c  SVGA 25fps 基础配置 + JPEG 模式配置（含量化表，0x44=0x20 质量）
drivers/sccb.c, i2c.c    软件 SCCB；i2c.c 里有 SWJ_JTAGDisable（PB3/PB4 可用前提）
drivers/uart.c           UART1 TX DMA（DMA1_CH4，无中断，1.5Mbaud）
middleware/usb/src|inc   ST USB-FS-Device 库 V4.1.0（勿改）
middleware/usb/uvc/      UVC 应用层（本项目核心增量）：
  usb_conf.h             PMA 地址、EP_NUM、IMR_MSK、EP 回调映射（EP1_IN_Callback→uvc_ep1_in_callback）
  usb_desc.c/.h          全部描述符（配置总长 189，VC 部 39，VS 部 99）
  usb_prop.c/.h          DEVICE_PROP/Device_Table/User_Standard_Requests、probe/commit、EP 初始化
  usb_pwr.c/.h           PowerOn/Suspend/Resume（简化版，不做时钟门控）、DEVICE_STATE 枚举
  usb_istr.c/.h          USB_Istr 事件分发（CTR/RESET/SUSP/WKUP/ESOF；SOF 未启用）
  usb_bsp.c/.h           USB 时钟/NVIC、usb_irq_lock/unlock（NVIC 关/开 USB 中断）、U_ID 序列号
  usbd_uvc.c/.h          ★ 流式状态机：load_next_packet（CTR 驱动装包）、静默模式、commit 解析
docs/app-manual.md       人读应用手册
README.md / README.zh-CN.md  英文/简体中文说明
```

## 4. 硬件事实（RM0008 Rev 21 查证，违反即坏）

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
- EP1 寄存器操作只发生在 USB 中断上下文（enable/disable/reset 也都源自该上下文），无并发。
- 帧长上限 12KB（单缓冲容量 + 描述符 dwMaxVideoFrameBufferSize），超限帧在采集层丢弃。
- UART 模式 RAM 裕量仅 ~540B：给 UART 模式加全局变量前先核对总量。

## 6. 已修复的关键缺陷（教训存档）

1. **黑屏 v1（已修复）**：ISO 按单缓冲实现 + 每包写 NAK → 违反上述硬件事实 1/2。
   症状：设备管理器正常、Windows 相机黑屏。修复 = 双缓冲 + DTOG 驱动 + STAT 恒 VALID。
2. SOF 中断装包存在"赶不上主机同帧 IN 轮询"的帧内丢包风险 → 改为 CTR_TX 驱动
   （发完立即装下一包，提前近 1ms），SOF 从中断屏蔽中移除。
3. 双缓冲帧缓冲（UART 时代）在 UVC 串行流水线下无意义，改单 12KB，腾出 RAM。
4. `usb_lib.h` 需要应用层提供 `hw_config.h`（垫片只含 `#include "main.h"`）。
5. 类请求码（GET_CUR 0x81 等）不在库头文件里，需在 `usb_prop.h` 自行定义。

## 7. 可选优化方向（未做，按价值排序）

1. **带宽提升**：BUF0/BUF1 指向同一 PMA 地址 + 保留各自计数，理论上可回到 368B/包
   （需实测硬件是否接受同地址双缓冲；收益：320x240 提升到约 12~15fps）。
2. EP0 缓冲压缩（RX/TX 各 32B）可再给 ISO 腾 64B。
3. 采集提速：OV2640 换更高帧率配置（当前 SVGA 25fps 上限）。
4. JPEG DHT 注入（若移植到不带量化/Huffman 表的传感器配置时才需要）。
5. UVC 1.1 + dwClockFrequency/PTS 时间戳（主机端更平滑）。

## 8. 验证清单（改动后执行）

1. `UV4 -r` 全量构建 UVC 默认配置：0 错 0 警；记录 Program Size 对比 RAM 裕量。
2. 切 `APP_TRANSPORT_UART` 构建，RAM ≤ 19536B 左右；改回 UVC 再构建。
3. 烧录（J-Link），设备管理器出现"摄像头"；Windows 相机应用出图。
4. 属性页确认 320x240/160x120 与 30/15fps 档位可选、切换后出图。
5. 若黑屏：按 `docs/app-manual.md` 第 10 节表格排查；USBPcap 抓 EP 0x81 确认包流。
