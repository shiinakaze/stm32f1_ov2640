#include "app.h"
#include "usb_bsp.h"
#include "usb_desc.h"

/* USB 时钟与中断初始化。
 * 系统时钟 72MHz（HSE×9），USBCLK = PLLCLK/1.5 = 48MHz。
 * 注意：128MHz 超频与 USB 不兼容（无法从 PLL 分频出 48MHz）。 */
void usb_bsp_init(void)
{
    NVIC_InitTypeDef nvic_init_struct;

    RCC_USBCLKConfig(RCC_USBCLKSource_PLLCLK_1Div5);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USB, ENABLE);

    /* USB 低优先级中断（CTR/SOF/复位/挂起等事件共用） */
    nvic_init_struct.NVIC_IRQChannel = USB_LP_CAN1_RX0_IRQn;
    nvic_init_struct.NVIC_IRQChannelPreemptionPriority = 0;
    nvic_init_struct.NVIC_IRQChannelSubPriority = 0;
    nvic_init_struct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic_init_struct);
}

/* OV2640 采集为 GPIO 位扫描，对中断延迟极其敏感：
 * 采集期间在 NVIC 层关闭 USB 中断，事件照常挂起，采集结束后统一处理。
 * 挂起期间硬件自动 NAK，主机只是观察到帧间无数据（合法的 UVC 行为）。 */
void usb_irq_lock(void)
{
    NVIC_DisableIRQ(USB_LP_CAN1_RX0_IRQn);
}

void usb_irq_unlock(void)
{
    NVIC_EnableIRQ(USB_LP_CAN1_RX0_IRQn);
}

/* 用芯片 96 位唯一 ID 生成序列号字符串描述符（12 个十六进制字符） */
void uvc_get_serial_num(void)
{
    uint32_t w0, w2;
    static const char hex[] = "0123456789ABCDEF";
    uint8_t i;

    w0 = *(volatile uint32_t *)0x1FFFF7E8 ^ *(volatile uint32_t *)0x1FFFF7EC;
    w2 = *(volatile uint32_t *)0x1FFFF7F0 ^ *(volatile uint32_t *)0x1FFFF7EC;

    uvc_string_serial[0] = 2 + 2 * 12;
    uvc_string_serial[1] = 0x03;
    for (i = 0; i < 12; i++)
    {
        uint8_t nibble = (i < 8) ? (uint8_t)(w0 >> (28 - 4 * i))
                                 : (uint8_t)(w2 >> (28 - 4 * (i - 8)));
        uvc_string_serial[2 + 2 * i] = (uint8_t)hex[nibble & 0x0F];
        uvc_string_serial[3 + 2 * i] = 0x00;
    }
}
