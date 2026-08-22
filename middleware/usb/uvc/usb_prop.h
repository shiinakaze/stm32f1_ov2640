#ifndef __USB_PROP_H
#define __USB_PROP_H

#include "usb_lib.h"

/* UVC 类特定请求码 */
#define SET_CUR (0x01)
#define GET_CUR (0x81)
#define GET_MIN (0x82)
#define GET_MAX (0x83)
#define GET_DEF (0x87)

void uvc_device_init(void);
void uvc_device_reset(void);
void uvc_device_status_in(void);
void uvc_device_status_out(void);
RESULT uvc_device_data_setup(uint8_t request_no);
RESULT uvc_device_no_data_setup(uint8_t request_no);
RESULT uvc_device_get_interface_setting(uint8_t interface, uint8_t alternate);
uint8_t *uvc_device_get_device_descriptor(uint16_t length);
uint8_t *uvc_device_get_config_descriptor(uint16_t length);
uint8_t *uvc_device_get_string_descriptor(uint16_t length);

#endif /* __USB_PROP_H */
