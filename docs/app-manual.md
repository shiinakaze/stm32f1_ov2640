# STM32F103 + OV2640 UVC 摄像头应用手册

本手册讲解 `stm32f1_ov2640` 工程如何把"OV2640 采集 MJPEG"改造成"USB 摄像头（UVC 设备）"，
内容按手动编写顺序组织——你可以不看源码、只按本手册从零复现整个实现。
每一节末尾给出对应实现文件作参照。

---

## 1. 原理总览

- **MCU**：STM32F103C8（Cortex-M3，20KB SRAM / 64KB Flash，USB 全速设备）
- **传感器**：OV2640（DSP 输出 MJPEG，DVP 并口，软件 SCCB 配置）
- **目标**：枚举为 UVC 1.0 MJPEG 摄像头，Windows/Linux 内置驱动直接出图

数据通路只有一条主线：

```
OV2640 --(GPIO位扫描采集)--> SRAM 帧缓冲 --(CTR_TX中断逐包)--> PMA --(ISO IN)--> 主机
```

USB 侧只需要两个端点：

| 端点 | 类型 | 用途 |
|---|---|---|
| EP0 | 控制 | 枚举、probe/commit 协商、SET_INTERFACE |
| EP1 IN | 同步（ISO），184B/包 | 视频流 |

> **为什么是 184B？** F103 的包缓冲区（PMA）总共只有 512 字节，EP0 收发各占 64B 后，
> 剩余 368B 平分给 ISO 的**两个**缓冲（见第 8 节），即 2×184B。

---

## 2. 资源与引脚规划

### 引脚（与 UART/OLED/SCCB 共存）

| 功能 | 引脚 |
|---|---|
| OV2640 数据 D0-D7 | PA0-PA7 |
| VSYNC / HREF / RESET | PB3 / PB4 / PB5 |
| PCLK / PWDN | PB8 / PB9 |
| USB DM / DP | PA11 / PA12（固定，不可改） |
| SCCB | 软件 SCCB（见 `drivers/sccb.c`） |
| UART1 TX（调试通道） | PA9 |

要点：

- PB3/PB4 默认是 JTAG 引脚，须先 `GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE)`
  （保留 SWD）——本工程在 `drivers/i2c.c` 中完成。
- PA11/PA12 只需使能 USB 时钟，外设自动接管，无需 GPIO 配置。
- **Blue Pill 必须有 D+ 1.5kΩ 上拉（R10）**，部分板子缺件会导致主机根本不开始枚举。

### 时钟（硬约束）

- 系统必须 **72MHz**（HSE 8MHz × 9）：USBCLK = PLLCLK / 1.5 = 48MHz。
- 128MHz 之类的超频无法分频出 48MHz，**UVC 模式不支持超频**。

---

## 3. USB 协议栈与中断接入

使用 ST 经典 USB-FS-Device 库（本仓库 `middleware/usb/src` + `inc`，V4.1.0）。
库分两层：

- **核心层**（不改）：`usb_regs.c`（端点/PMA 寄存器操作）、`usb_core.c`（标准请求解析）、
  `usb_int.c`（CTR 分发）、`usb_init.c`、`usb_mem.c`（PMA 拷贝）、`usb_sil.c`
- **应用层**（自己写，放 `middleware/usb/uvc/`）：
  `usb_conf.h` / `usb_desc.c` / `usb_prop.c` / `usb_pwr.c` / `usb_istr.c` / `usbd_uvc.c`

应用层通过四个结构体挂进库：

```c
DEVICE Device_Table = { EP_NUM, 1 };          /* 端点数、配置数 */
DEVICE_PROP Device_Property = { ... };        /* Init/Reset/类请求/描述符回调 */
USER_STANDARD_REQUESTS User_Standard_Requests = { ... }; /* 标准请求用户钩子 */
```

初始化序列（main 中）：

```c
usb_bsp_init();   /* USBCLK=PLL/1.5，使能 USB 时钟，NVIC 使能 USB_LP_CAN1_RX0_IRQn */
USB_Init();       /* 库入口 → Device_Property.Init() → PowerOn + USB_SIL_Init */
```

中断服务函数加在 `app/stm32f10x_it.c`：

```c
void USB_LP_CAN1_RX0_IRQHandler(void) { USB_Istr(); }
```

`USB_Istr()`（自己写，`usb_istr.c`）依次处理 ISTR 的 CTR/RESET/SUSP/WKUP/ESOF 事件。
SOF 中断不需要——ISO 发送由 CTR_TX 驱动（见第 6 节），屏蔽字里可以不放 `CNTR_SOFM`，
省掉每毫秒一次的中断。

> 坑：`usb_lib.h` 第 44 行 `#include "hw_config.h"` 是给 ST 官方例程留的应用头，
> 需要自己提供一个（内容只需 `#include "main.h"`）。

---

## 4. 描述符编写

全部在 `usb_desc.c`，用字节数组硬编码。**所有多字节字段小端**。

### 4.1 设备描述符（18B）

```
0x12, 0x01, 0x00,0x02,   /* bLength, bDescriptorType, bcdUSB=2.00 */
0xEF, 0x02, 0x01,        /* 类 0xEF/0x02/0x01 = Misc/Common/IAD 复合设备约定 */
0x40,                    /* bMaxPacketSize0 = 64 */
VID(2), PID(2), bcdDevice(2),
iManufacturer=1, iProduct=2, iSerialNumber=3, bNumConfigurations=1
```

用 IAD 组织视频功能时设备类必须填 0xEF/0x02/0x01，否则 Windows 不认。

### 4.2 配置描述符骨架（总长 189B）

```
Config(9) + IAD(8)
+ [VC 接口: Iface(9) + VC头(13) + 输入端子(17) + 输出端子(9)]   ← VC 类部总长 39
+ [VS alt0: Iface(9) + VS输入头(14) + MJPEG格式(11) + 帧1(34) + 帧2(34) + 颜色匹配(6)]  ← VS 类部总长 99
+ [VS alt1: Iface(9) + ISO端点(7)]
```

两个字节数必须自己算对并填进对应 wTotalLength：

- VC 头的 wTotalLength = 13+17+9 = **39**
- VS 输入头的 wTotalLength = 14+11+34+34+6 = **99**
- Config 的 wTotalLength = 9+8+9+39+9+99+9+7 = **189**

**长度自检方法**：把每段的 bLength 求和，必须等于三个 wTotalLength——描述符错一个字节，
主机就解析失败或设备管理器黄色叹号。

### 4.3 帧描述符（30B→34B 的由来）

帧描述符长度公式：`bLength = 26 + 4 × bFrameIntervalType`。

每个分辨率一个帧描述符。双帧率档（30/15fps）写法（以 320x240 为例）：

```
0x22,                    /* bLength = 26 + 4×2 = 34 */
0x24, 0x07,              /* CS_INTERFACE, VS_FRAME_MJPEG */
0x01,                    /* bFrameIndex = 1 */
0x00,                    /* bmCapabilities */
0x40,0x01, 0xF0,0x00,    /* wWidth=320, wHeight=240 */
dwMinBitRate(4)          /* = W×H×2×fps_min×8 */
dwMaxBitRate(4)          /* = W×H×2×fps_max×8 */
dwMaxVideoFrameBufferSize(4)  /* 12288 = 帧缓冲容量，超出的帧会被固件丢弃 */
dwDefaultFrameInterval(4)     /* 666666（15fps，单位 100ns） */
0x02,                    /* bFrameIntervalType = 2 个离散帧率 */
dwFrameInterval[0](4)    /* 333333（30fps）← 离散帧率必须升序排列 */
dwFrameInterval[1](4)    /* 666666（15fps） */
```

帧率换算：`interval = 10^7 / fps`（100ns 单位）。30fps=333333，15fps=666666。

160x120 为 bFrameIndex=2，结构相同。切换分辨率时主机重新 commit + SET_INTERFACE，
固件在 commit 里记下 bFrameIndex，主循环下一次采集前调 `ov2640_set_output_size()` 生效。

### 4.4 ISO 端点描述符（7B）

```
0x07, 0x05, 0x81,        /* ENDPOINT, EP1 IN */
0x01,                    /* ISO，无同步，数据端点 */
0xB8, 0x00,              /* wMaxPacketSize = 184 */
0x01                     /* bInterval = 每帧一个包 */
```

### 4.5 字符串描述符

LangID(0x0409)、厂商、产品为常量数组；序列号用芯片唯一 ID（0x1FFFF7E8 起 3 个字）
运行时生成（`usb_bsp.c` 的 `uvc_get_serial_num`），避免多板同 PID 冲突。

---

## 5. 控制请求处理（`usb_prop.c`）

### 5.1 库的回调模型

- 标准请求（SET_CONFIG / GET_DESCRIPTOR / SET_INTERFACE…）由 `usb_core.c` 处理，
  通过 `Class_Get_Interface_Setting` 校验合法性，再调 `User_SetInterface` 等钩子。
- 类请求（带数据阶段）路由到 `Class_Data_Setup(bRequest)`。

### 5.2 接口/备用设置校验

```c
RESULT uvc_device_get_interface_setting(uint8_t iface, uint8_t alt)
{
    if (iface == 0) return (alt == 0) ? USB_SUCCESS : USB_UNSUPPORT;
    if (iface == 1) return (alt <= 1)  ? USB_SUCCESS : USB_UNSUPPORT;
    return USB_UNSUPPORT;
}
```

主机用 `SET_INTERFACE(接口1, alt=1)` 起流、`alt=0` 停流，钩子里调
`uvc_stream_enable()/uvc_stream_disable()`。

### 5.3 probe/commit 协商

UVC 协商序列：主机 GET_DEF/GET_CUR probe → SET_CUR probe → GET_CUR probe → SET_CUR commit →
SET_INTERFACE(alt1)。probe/commit 各是 **26 字节**（UVC 1.0）：

```
偏移  长度  字段
0     2    bmHint
2     1    bFormatIndex        ← 分辨率切换依据
3     1    bFrameIndex
4     4    dwFrameInterval
8     10   各速率/质量字段
18    4    dwMaxVideoFrameSize
22    4    dwMaxPayloadTransferSize  ← 单包最大载荷（含 2B 头）
```

**不要用结构体映射**（uint32 在偏移 18 处会因对齐被填充），用字节数组 + 宏读写。

CopyRoutine 模式（库的标准姿势）：

```c
static uint8_t *probe_command(uint16_t length)
{
    if (length == 0) {                        /* 第一次调用：上报总长 */
        pInformation->Ctrl_Info.Usb_wLength = 26;
        return NULL;
    }
    return &uvc_probe_data[length];           /* 之后按偏移取缓冲 */
}
```

GET_CUR/GET_DEF/GET_MIN/GET_MAX 都返回同一份当前值即可；另需实现 GET_INFO
（1 字节 0x03 = 支持 GET/SET）和 GET_LEN（2 字节 26）。

请求过滤：`bmRequestType` 必须是"类 + 接口接收者"，且 wIndex 低字节 = 1（VS 接口号）、
wValue 高字节 = 0x01（probe）/ 0x02（commit）。

---

## 6. ISO 发送引擎（`usbd_uvc.c`）——本工程最核心的一节

### 6.1 RM0008 §23.4.4 的三条硬事实（血泪教训）

1. **F1 的 ISO 端点硬件上就是双缓冲的**：每次 IN 传输完成后，硬件自动翻转 DTOG_TX，
   交替使用两组缓冲描述符——BUF0（TX 槽：ADDR_TX/COUNT_TX）和 BUF1（RX 槽：
   ADDR_RX/COUNT_RX）。**没有"单缓冲 ISO"可用**：只初始化一组描述符，
   第二个包起硬件就读垃圾地址 → 主机只收到 1 个包，画面永远黑屏（枚举却完全正常）。
2. **STAT_TX 只允许 DISABLED(00)/VALID(11)**，传输完成后硬件保持 VALID 不变。
   **对 ISO 端点写 NAK 是未定义行为**——不要用 bulk 端点那套 NAK→VALID 的重装流程。
3. 每次传输结束 CTR_TX 置位，DTOG_TX 已翻转到"下一个要用的缓冲"。

Table 171（IN 端点）语义：**DTOG_TX=0 → 硬件下一个发 BUF0；=1 → 发 BUF1**。

### 6.2 引擎逻辑（每次 CTR_TX 做一次）

```
读 DTOG_TX → 得知硬件下一个用哪个缓冲（假设为 B）
拼包：pkt[0]=2, pkt[1]=FID|(最后一包?EOF:0), 后接最多 182B 帧数据
UserToPMABufferCopy(pkt → B 的地址)
SetEPDblBuf{0|1}Count(ENDP1, EP_DBUF_IN, 包长)      ← 注意 IN 方向是裸计数直写
不碰 STAT、不碰 DTOG
```

FID 规则：每帧的**最后一包**带 EOF 位并同时翻转 FID；随后的空包和下一帧数据
用同一个新 FID——主机视角每帧恰好翻转一次。

空包（帧间）：仅 2 字节头 `{2, FID}`，无数据无 EOF。这是真实摄像头帧间的标准行为。

### 6.3 启流 / 停流

```c
uvc_stream_enable():   /* SET_INTERFACE(alt1) */
    ClearDTOG_RX(ENDP1); ClearDTOG_TX(ENDP1);   /* 硬件从 BUF0 开始 */
    装一个空包进 BUF0; SetEPDblBuf0Count(..., 2);
    SetEPTxStatus(ENDP1, EP_TX_VALID);          /* 此后不再写 STAT */

uvc_stream_disable():  /* alt0 / 复位 / 挂起 */
    SetEPTxStatus(ENDP1, EP_TX_DIS);            /* DIS 合法 */
```

### 6.4 静默模式（采集期间的空包流）

一帧 EOF 包**发送完成**的那次 CTR_TX 里：

```
两个缓冲此刻都归软件 → 各装入一个空包
```

之后主循环锁掉 USB 中断去采集（见第 7 节），硬件靠这两包自动交替发送，
总线上持续保持规整的空包流，**零 CPU 参与**。采集结束解锁后，
挂起的 CTR（多个事件折叠为一个 CTR 位）恢复装包，按 DTOG_TX 读到的当前值
判断该装哪个缓冲即可——软件不需要自己数缓冲序号，硬件 DTOG 永远是真相。

---

## 7. 采集与 USB 的并发（`main.c` + `drivers/ov2640.c`）

### 7.1 为什么不能并发

OV2640 采集是 `while(PCLK==0); 读 GPIO; while(PCLK==1);` 的位扫描轮询，
任何中断（哪怕 1µs）都会丢字节、毁掉整帧。而 UVC 流需要每毫秒装一个包。
二者只能**串行**：

```
等上一帧 EOF 包发出（frame_done 置位）
→ CTR 里已把两个缓冲装成空包（静默模式）
→ NVIC 关 USB 中断（NVIC_DisableIRQ(USB_LP_CAN1_RX0_IRQn)）
→ ov2640_capture()          /* 约 40ms，VSYNC 帧周期 */
→ NVIC 开中断（挂起事件立即得到处理：折叠的 CTR + 可能的 SETUP 等）
→ ov2640_get_ready_frame() → uvc_feed_frame()
```

帧节奏 ≈ 40ms 采集 + N×1ms 发送（12KB 帧 ≈ 68 包）：
320x240 实际约 10~13fps，160x120 约 15~18fps。

### 7.2 单缓冲即够

串行流水线里采集时无人读缓冲 → 单 12KB 缓冲。
（UART 调试通道是另一套：双 8KB，采集与上一帧 DMA 并行，见 `APP_TRANSPORT` 宏。）

### 7.3 采集期间主机看到什么

帧间 40ms 空包流 + 正常数据包。对主机来说这是一帧"产生得很慢"的视频，
帧内零丢包——完全合法的 UVC 行为。

---

## 8. PMA 布局计算（512B 用满）

PMA 前 64 字节是 BTABLE（8 个端点 × 8B）。只用 EP0/EP1 时，
**EP2-EP7 的表项区（0x10-0x3F，48B）是死空间，可以拿来做缓冲**：

```
0x000  BTABLE 有效表项（EP0、EP1）          16B
0x010  EP0 RX 缓冲 64B（前 48B 复用死空间）
0x050  EP0 TX 缓冲 64B
0x090  EP1 BUF0 184B                        ← TX 描述符槽
0x148  EP1 BUF1 184B                        ← RX 描述符槽
0x200  ← 正好 512B 用完
```

计算式：512 − 16(BTABLE 有效) − 128(EP0) = 368 → 2×184。
`usb_conf.h` 里定义地址，`usb_prop.c` 的 Reset 里 `SetEPDblBuffAddr` 一次设好。

---

## 9. 上电与电源管理

- **挂起（SUSP）**：置 `CNTR_FSUSP`、停流、状态标 SUSPENDED。不做时钟门控/休眠
  （相机随时要采）。
- **唤醒（WKUP）**：清 FSUSP、恢复状态。主机一般会重新 SET_INTERFACE 起流。
- ESOF 里保留 ST 例程的"连续 3 次 ESOF 且 D+ 为高 → 强制复位恢复"逻辑，
  防止丢失 SUSP 标志卡死。

---

## 10. 常见坑速查（按出现频率）

| 症状 | 原因 |
|---|---|
| 设备管理器不出现 | Blue Pill 缺 R10 上拉；PA11/PA12 被占用 |
| 枚举正常，相机黑屏 | ISO 按单缓冲写了（只设 ADDR_TX）；或对 ISO 写了 NAK |
| 黑屏且偶尔闪一帧 | DTOG/缓冲选择逻辑错，半数包是垃圾 |
| 能出图但频繁坏帧 | 采集期间没锁 USB 中断（丢字节）；或分辨率>缓冲容量被截断 |
| 主机报无法启动视频 | probe/commit 长度或 GET_LEN 应答错；帧描述符 wTotalLength 算错 |
| 解码花屏但帧完整 | OV2640 JPEG 缺 DHT/量化表（本工程配置已含表；移植别家配置时注意） |

---

## 11. 手动编码检查清单

- [ ] 72MHz 时钟，`RCC_USBCLKConfig(PLL/1.5)`
- [ ] `hw_config.h` 垫片（usb_lib.h 需要）
- [ ] 描述符三个 wTotalLength 求和自检
- [ ] 设备类 0xEF/0x02/0x01（IAD）
- [ ] ISO 端点 `SetEPDblBuffAddr` 两组地址 + `ClearDTOG` + 只写 DIS/VALID
- [ ] CTR_TX 回调按 DTOG_TX 选缓冲，`SetEPDblBuf{0,1}Count(EP_DBUF_IN, n)`
- [ ] EOF 包发完 → 双缓冲装空包 → 才允许锁中断采集
- [ ] feed 顺序：指针 → 标志 → 长度
- [ ] `stm32f10x_it.c` 挂 `USB_LP_CAN1_RX0_IRQHandler`
- [ ] PB3/PB4 用前关 JTAG

---

*实现参照：`middleware/usb/uvc/`（应用层）、`app/main.c`（流水线）、
`drivers/ov2640.c`（采集）、`middleware/usb/Projects/STM32F407-UVC/`（F4 参考工程，
仅描述符结构可参考，端点驱动模型完全不同）。*
