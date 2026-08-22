**English** | [简体中文](README.zh-CN.md)

branch:

- main: UVC webcam (USB video class device)
- stm32f1_ov2640_test: test signal
- stm32f1_ov2640_dma: legacy (UART output version)

## Documentation

- [Application manual](docs/app-manual.md) — how the firmware works, step by step (for manual re-implementation)

## Overview

STM32F103C8 (Blue Pill) + OV2640: captures MJPEG and supports two transport channels (compile-time switch):

- **UVC device** (default): enumerates as a USB webcam on Windows / Linux, no host software required
- **UART transport**: 1.5 Mbaud output for the receiver script `tools/main.py` (debug)

### Switching transport

Select in `app/main.h` (rebuild after changing):

```c
#define APP_TRANSPORT APP_TRANSPORT_UVC   // or APP_TRANSPORT_UART
```

- UVC mode: single 12 KB frame buffer, USB interrupts locked during capture (see below)
- UART mode: dual 8 KB buffers, capture runs in parallel with the DMA transfer of the previous frame (same as the legacy version)

### Architecture

```
OV2640 (DVP, GPIO bit-bang)           STM32F103C8 @72MHz               PC
┌──────────────┐   VSYNC/HREF/PCLK  ┌─────────────────┐   USB FS ISO IN   ┌──────────┐
│ 320x240/160x120│ ───────────────► │ JPEG frame 12KB │ ────────────────► │ UVC host  │
│ MJPEG 30/15fps│    PA0-7/PB3-9    │ EP1 ISO dbl-buf │   EP 0x81, 184B   │ camera app│
└──────────────┘                    └─────────────────┘                   └──────────┘
```

- USB stack: ST classic USB-FS-Device library (`middleware/usb/src`)
- UVC class: `middleware/usb/uvc/` (descriptors, probe/commit negotiation, CTR-driven packet engine)
- Firmware must run at 72 MHz (USBCLK = PLL/1.5 = 48 MHz); 128 MHz overclocking is not supported

### Key design 1: double-buffered ISO semantics on F1 (RM0008 §23.4.4)

On the F1 USB peripheral, isochronous endpoints are double-buffered in hardware:
after each IN transaction the hardware toggles DTOG_TX and alternates between
BUF0 (TX descriptor slot) and BUF1 (RX descriptor slot); STAT_TX stays VALID
once armed (writing NAK to an ISO endpoint is undefined behavior).
The transmit engine fills, on every CTR_TX (right after a packet goes out),
the buffer the hardware will use next, as indicated by DTOG_TX.

### Key design 2: serial pipeline between capture and USB

OV2640 capture is a GPIO bit-banged poll loop; any interrupt corrupts the
byte stream. USB interrupts are therefore masked at the NVIC level while
capturing (latched events are serviced after unlocking):

```
wait for previous frame EOF on the bus → load null packets into both buffers
  → lock USB interrupts → capture (~40ms)
  → unlock → uvc_feed_frame → CTR_TX sends one 184B packet per ms until EOF ─┐
                                                                             ┘
```

- During capture the hardware autonomously alternates the two pre-loaded
  UVC null packets (2-byte payload header only), keeping a well-formed
  null-packet stream on the bus with zero CPU involvement — exactly what a
  real webcam does between frames
- Host view: one "slowly produced" frame (nulls + data + EOF), no lost
  packets inside a frame
- Resolutions/frame rates: descriptors advertise 320x240 and 160x120, each
  with 30/15 fps options (selectable in the camera properties); the selected
  rate paces the host renderer, actual output is pipeline-limited to roughly
  10–13 fps (320x240) / 15–18 fps (160x120)
- Frames larger than 12 KB (dwMaxVideoFrameBufferSize in the descriptors) are dropped

### PMA layout (512B)

| Region | Address | Size |
| ------ | ------- | ---- |
| BTABLE (EP0/EP1 entries) | 0x000 | 16B |
| EP0 RX buffer (reuses free entries) | 0x010 | 64B |
| EP0 TX buffer | 0x050 | 64B |
| EP1 ISO IN BUF0 (TX slot) | 0x090 | 184B |
| EP1 ISO IN BUF1 (RX slot) | 0x148 | 184B |

### Hardware requirements

- USB uses PA11 (DM) / PA12 (DP); no conflict with camera pins (PA0-7 data, PB3/4/5/8/9 control)
- Blue Pill boards need the D+ 1.5k pull-up (some low-quality clones miss R10 — enumeration won't start without it)
- VID/PID are test values 0x1234/0x5678; Windows/Linux bind the in-box UVC driver by class code

### Resource usage (armcc -O1)

- Flash: ~16.4KB / 64KB
- RAM: ~15.5KB / 20KB (including the 12KB JPEG frame buffer); UART mode: ~19.6KB / 20KB

Thanks AI IDE for the help.

Thanks GLM for the help.
