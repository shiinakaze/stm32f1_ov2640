#include "main.h"
#include "ov2640.h"
#include "oled.h"
#include "delay.h"

#if APP_TRANSPORT == APP_TRANSPORT_UVC
#include "usb_lib.h"
#include "usb_bsp.h"
#include "usbd_uvc.h"

/* 帧缓冲：串行流水线（采集→USB 发送→再采集）下单缓冲即可。
 * 16KB 覆盖 640x360 MJPEG 常见帧大小，320x240 典型仅 5~10KB；
 * 超出缓冲的帧被丢弃（采集截断、无 EOI）。
 * RAM 预算：F103C8 共 20KB，本缓冲占 16KB，其余静态数据约 3.3KB。 */
#define JPEG_BUFFER_SIZE (16 * 1024)
static uint8_t jpeg_buffer[JPEG_BUFFER_SIZE];
#endif

#if APP_TRANSPORT == APP_TRANSPORT_UART
#include "uart.h"

/* 双缓冲：采集写入一块的同时，UART DMA 正在发送另一块 */
#define JPEG_BUFFER_SIZE (8 * 1024)
static uint8_t jpeg_buffer[2][JPEG_BUFFER_SIZE];
#endif

int main(void)
{
    ov2640_err_t err;
    uint16_t pid, mid;
    uint8_t *frame;
    uint32_t len;
#if APP_TRANSPORT == APP_TRANSPORT_UVC
    uint16_t width, height;
    uint32_t frames_ok = 0;      /* 已成功送出的帧数 */
    uint32_t frames_dropped = 0; /* 采集失败被丢弃的帧数 */
#endif
#if APP_TRANSPORT == APP_TRANSPORT_UART
    uint8_t capture_idx = 0;
#endif

    /* 保持 SystemInit 的 72MHz：UVC 需要 PLL/1.5 = 48MHz USB 时钟，
     * 128MHz 超频与 USB 不兼容。 */

    oled_init();
    err = ov2640_init();
    if (err != OV2640_OK)
    {
        /* 传感器缺失/ID 不符/配置校验失败：显示错误码并停机，
         * 继续运行只会流出无效数据 */
        oled_show_string(1, 1, (uint8_t *)"OV2640 init err");
        oled_show_hex_num(2, 1, err, 2);
        while (1)
        {
        }
    }

#if APP_TRANSPORT == APP_TRANSPORT_UVC
    ov2640_set_frame_buffer(jpeg_buffer, JPEG_BUFFER_SIZE);
#endif

#if APP_TRANSPORT == APP_TRANSPORT_UART
    uart1_init(1500000);
#endif

    if (ov2640_get_pid(&pid) == OV2640_OK)
    {
        oled_show_hex_num(1, 1, pid, 4);
    }
    if (ov2640_get_mid(&mid) == OV2640_OK)
    {
        oled_show_hex_num(2, 1, mid, 4);
    }

#if APP_TRANSPORT == APP_TRANSPORT_UVC
    /* OLED 第 3/4 行实时诊断（流式期间）：
     * F = 已送出帧数，E = 丢弃帧数（标记缺失/截断/超时）。
     * F 增长而画面黑 → 问题在 USB/主机侧；E 增长 → 采集侧丢帧 */
    oled_show_string(3, 1, (uint8_t *)"F:");
    oled_show_string(4, 1, (uint8_t *)"E:");

    usb_bsp_init();
    USB_Init();

    while (1)
    {
        if (!uvc_is_streaming())
        {
            continue;
        }

        /* 等上一帧的 EOF 包发出（总线进入空包流），缓冲可安全覆盖 */
        while (uvc_is_streaming() && !uvc_frame_done())
        {
        }
        if (!uvc_is_streaming())
        {
            continue;
        }

        /* 主机 commit 改变分辨率时，在采集前应用（SCCB 写入较慢） */
        if (uvc_poll_resolution(&width, &height))
        {
            ov2640_set_output_size(width, height);
        }

        /* 采集为 GPIO 位扫描，期间关闭 USB 中断防丢字节；
         * 挂起的 USB 事件在解锁后统一处理，采集期间硬件持续
         * 发送预装的空包，主机仅观察到帧间数据变慢 */
        usb_irq_lock();
        ov2640_capture();
        usb_irq_unlock();

        frame = ov2640_get_ready_frame(&len);
        if (frame != NULL && len > 0 && len <= 0xFFFF)
        {
            uvc_feed_frame(frame, (uint16_t)len);
            frames_ok++;
        }
        else
        {
            frames_dropped++;
        }
        oled_show_num(3, 3, frames_ok, 6);
        oled_show_num(4, 3, frames_dropped, 6);
    }
#else
    while (1)
    {
        /* 采集写入当前空闲缓冲（与上一次 DMA 发送并行） */
        ov2640_set_frame_buffer(jpeg_buffer[capture_idx], JPEG_BUFFER_SIZE);
        ov2640_capture();

        /* 等上一帧 DMA 发送完毕，再启动新帧发送 */
        while (!uart1_is_transmit_complete())
        {
        }

        frame = ov2640_get_ready_frame(&len);
        if (frame != NULL && len > 0)
        {
            uart1_transmit_non_blocking(frame, (uint16_t)len);
        }
        capture_idx ^= 1;
    }
#endif
}
