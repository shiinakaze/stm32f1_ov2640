[English](README.md) | **简体中文**

branch:

- main: UVC 摄像头（USB 视频类设备）
- stm32f1_ov2640_test: Test signal
- stm32f1_ov2640_dma: legacy（UART 输出版本）

## 文档

- [应用手册](docs/app-manual.md) — 固件原理与手动复现指南（面向人）

## 项目说明

STM32F103C8（Blue Pill）+ OV2640：采集 MJPEG，支持两种传输通道（编译期宏切换）：

- **UVC 设备**（默认）：USB 摄像头，Windows / Linux 直接识别，无需上位机软件
- **UART 传输**：1.5M 波特率，配合 `tools/main.py` 接收（调试用）

### 传输通道切换

`app/app.h` 中选择（改后重新编译）：

```c
#define APP_TRANSPORT APP_TRANSPORT_UVC   // 或 APP_TRANSPORT_UART
```

- UVC 模式：单 16KB 帧缓冲，采集期间锁 USB 中断（详见下文）
- UART 模式：双 8KB 缓冲，采集与上一帧的 DMA 发送并行（与旧版行为一致）

### 架构

```
OV2640 (DVP, GPIO 位扫描)            STM32F103C8 @72MHz              PC
┌──────────────┐   VSYNC/HREF/PCLK  ┌─────────────────┐   USB FS ISO IN   ┌──────────┐
│ 320x240/160x120/640x360│ ─────► │ JPEG 帧缓冲 16KB │ ────────────────► │ UVC 主机  │
│ MJPEG 30fps   │    PA0-7/PB3-9    │ EP1 ISO 双缓冲   │   0x81 端点 184B   │ 相机应用  │
└──────────────┘                    └─────────────────┘                   └──────────┘
```

- USB 协议栈：ST 经典 USB-FS-Device 库（`middleware/usb/src`）
- UVC 类实现：`middleware/usb/uvc/`（描述符、probe/commit 协商、CTR 驱动的打包引擎）
- 固件必须运行在 72MHz（USBCLK = PLL/1.5 = 48MHz），不支持 128MHz 超频

### 目录结构

按嵌入式 SDK 目录结构设计规范（AN-SDK-001）分层：

```
arch/arm/cortex-m3/       CMSIS core（CPU 架构层）
chip/stm32f103/           器件头文件、系统初始化、启动文件、SPL 厂商外设库（spl/）
boards/bluepill/          board_config.h —— 引脚/外设实例选择宏
drivers/peripheral/       自研芯片外设驱动：timer、sw_i2c
drivers/device/           外部器件驱动：ov2640 摄像头（含 sccb/ 总线）、ssd1306 显示、sht30 传感器
middleware/               usb（ST USB 库 + UVC 类实现）
app/                      应用入口 app_main()、传输通道切换
examples/vendor/          ST 参考工程（不参与编译）
kernel/baremetal/         裸机引导：main() 引导进入 app_main()
tests/                    单元 / 集成测试（预留）
tools/                    PC 端接收脚本与内存分析报告
build/                    Keil 编译输出（obj/、map/），已 gitignore
```

### 关键设计 1：F1 ISO 端点的双缓冲语义（RM0008 §23.4.4）

F1 USB 外设的同步端点**硬件上就是双缓冲**：每次 IN 传输完成后硬件
自动翻转 DTOG_TX，交替使用 BUF0（TX 描述符槽）/ BUF1（RX 描述符槽）
两个缓冲；STAT_TX 启动后保持 VALID（对 ISO 写 NAK 是未定义行为）。
发送引擎在每次 CTR_TX（一包刚发完）时，按 DTOG_TX 把下一包装进
硬件即将使用的那个缓冲。

### 关键设计 2：采集与 USB 的串行流水线

OV2640 采集为 GPIO 位扫描轮询，任何中断都会导致丢字节。
因此采集期间在 NVIC 层关闭 USB 中断（挂起的事件在解锁后统一处理）：

```
等上一帧 EOF 发出 → 两个缓冲装入空包 → 锁 USB 中断 → 采集(约40ms)
                                                              │
   解锁 → uvc_feed_frame → CTR_TX 每毫秒交替装包发送整帧 ──────┘
```

- 采集期间硬件自动交替发送两个预装的 UVC 空包（仅 2 字节载荷头），
  总线上保持规整的空包流，无需 CPU 参与——这正是真实摄像头的行为
- 主机视角：一帧"慢速产生"的视频（空包 + 数据 + EOF），帧内无丢包
- 分辨率/帧率：描述符声明 320x240、160x120 与 640x360，固定 30fps；
  帧率为主机渲染节奏，实际出帧受流水线限制，
  约 10~13fps（320x240）/ 15~18fps（160x120）/ 6~8fps（640x360，受发送带宽限制）
- 帧超过 16KB（描述符声明的 dwMaxVideoFrameBufferSize）会被丢弃

### PMA 布局（512B）

| 区域 | 地址 | 大小 |
| ---- | ---- | ---- |
| BTABLE（EP0/EP1 表项） | 0x000 | 16B |
| EP0 RX 缓冲（复用空闲表项） | 0x010 | 64B |
| EP0 TX 缓冲 | 0x050 | 64B |
| EP1 ISO IN BUF0（TX 槽） | 0x090 | 184B |
| EP1 ISO IN BUF1（RX 槽） | 0x148 | 184B |

### 硬件要求

- USB 使用 PA11 (DM) / PA12 (DP)，与相机引脚（PA0-7 数据、PB3/4/5/8/9 控制）无冲突
- Blue Pill 需要有 D+ 1.5k 上拉电阻（部分劣质板缺 R10，否则无法枚举）
- VID/PID 为测试值 0x1234/0x5678，Windows/Linux 依据 UVC 类代码加载内置驱动

### 资源占用（armcc -O1）

- Flash：约 16.5KB / 64KB
- RAM：约 19.1KB / 20KB（含 16KB JPEG 帧缓冲）；UART 模式约 19.6KB / 20KB

Thanks AI IDE for the help.
Thanks GLM for the help.
