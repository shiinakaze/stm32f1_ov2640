#ifndef __USB_BSP_H
#define __USB_BSP_H

#include <stdint.h>

void usb_bsp_init(void);
void usb_irq_lock(void);
void usb_irq_unlock(void);
void uvc_get_serial_num(void);

#endif /* __USB_BSP_H */
