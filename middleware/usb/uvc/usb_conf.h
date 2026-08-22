#ifndef __USB_CONF_H
#define __USB_CONF_H

/*-------------------------------------------------------------*/
/* EP_NUM: 设备使用的端点数（EP0 控制 + EP1 ISO IN）            */
/*-------------------------------------------------------------*/
#define EP_NUM (2)

/*-------------------------------------------------------------*/
/* PMA（共 512 字节）布局：                                     */
/*   0x000-0x00F  BTABLE 中 EP0/EP1 的有效表项                  */
/*   0x010-0x04F  EP0 RX 缓冲 64B（复用 EP2-EP7 的空闲表项区）   */
/*   0x050-0x08F  EP0 TX 缓冲 64B                               */
/*   0x090-0x147  EP1 ISO IN BUF0 184B（TX 描述符槽）           */
/*   0x148-0x1FF  EP1 ISO IN BUF1 184B（RX 描述符槽）           */
/*                                                              */
/* 注意 RM0008 §23.4.4：F1 的 ISO 端点硬件上就是双缓冲的，      */
/* 每次传输后硬件自动翻转 DTOG_TX，交替使用两组描述符。          */
/*-------------------------------------------------------------*/
#define BTABLE_ADDRESS (0x00)
#define ENDP0_RXADDR (0x10)
#define ENDP0_TXADDR (0x50)
#define ENDP1_BUF0ADDR (0x90)
#define ENDP1_BUF1ADDR (0x148)

/* ISO 包大小（含 2 字节 UVC 载荷头，单包最大视频载荷 182 字节） */
#define UVC_ISO_PACKET_SIZE (184)

/*-------------------------------------------------------------*/
/* 需要处理的 ISTR 事件屏蔽（SOF 无数据职责，不再开启，省 1kHz  */
/* 中断开销；ESOF 保留用于挂起检测恢复）                        */
/*-------------------------------------------------------------*/
#define IMR_MSK (CNTR_CTRM | CNTR_WKUPM | CNTR_SUSPM | CNTR_ERRM | \
                 CNTR_ESOFM | CNTR_RESETM)

/* EP1 IN（ISO）回调：发送完一包后立即装填下一包（usb_conf 中
 * 映射到 uvc_ep1_in_callback），其余非控制端点未使用 */
#define EP1_IN_Callback uvc_ep1_in_callback
#define EP2_IN_Callback NOP_Process
#define EP3_IN_Callback NOP_Process
#define EP4_IN_Callback NOP_Process
#define EP5_IN_Callback NOP_Process
#define EP6_IN_Callback NOP_Process
#define EP7_IN_Callback NOP_Process
#define EP1_OUT_Callback NOP_Process
#define EP2_OUT_Callback NOP_Process
#define EP3_OUT_Callback NOP_Process
#define EP4_OUT_Callback NOP_Process
#define EP5_OUT_Callback NOP_Process
#define EP6_OUT_Callback NOP_Process
#define EP7_OUT_Callback NOP_Process

#endif /* __USB_CONF_H */
