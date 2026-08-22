#ifndef __USB_DESC_H
#define __USB_DESC_H

#include <stdint.h>

#define UVC_SIZ_DEVICE_DESC (18)
#define UVC_SIZ_CONFIG_DESC (211)

#define UVC_SIZ_STRING_LANGID (4)
#define UVC_SIZ_STRING_VENDOR (2 + 2 * 4)    /* "Kaze" */
#define UVC_SIZ_STRING_PRODUCT (2 + 2 * 18)  /* "STM32F1 OV2640 UVC" */
#define UVC_SIZ_STRING_SERIAL (2 + 2 * 12)

extern const uint8_t uvc_device_descriptor[UVC_SIZ_DEVICE_DESC];
extern const uint8_t uvc_config_descriptor[UVC_SIZ_CONFIG_DESC];
extern const uint8_t uvc_string_lang_id[UVC_SIZ_STRING_LANGID];
extern const uint8_t uvc_string_vendor[UVC_SIZ_STRING_VENDOR];
extern const uint8_t uvc_string_product[UVC_SIZ_STRING_PRODUCT];
extern uint8_t uvc_string_serial[UVC_SIZ_STRING_SERIAL];

#endif /* __USB_DESC_H */
