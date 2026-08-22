#ifndef __USB_PWR_H
#define __USB_PWR_H

#include "usb_lib.h"

typedef enum _DEVICE_STATE
{
    UNCONNECTED,
    ATTACHED,
    POWERED,
    SUSPENDED,
    ADDRESSED,
    CONFIGURED
} DEVICE_STATE;

typedef enum _RESUME_STATE
{
    RESUME_EXTERNAL,
    RESUME_INTERNAL,
    RESUME_LATER,
    RESUME_KEEP,
    RESUME_NEXT,
    RESUME_REUSE
} RESUME_STATE;

extern __IO uint32_t bDeviceState;
extern __IO uint32_t fSuspendEnabled;

RESULT PowerOn(void);
RESULT PowerOff(void);
void Suspend(void);
void Resume(RESUME_STATE e_resume_set_val);

#endif /* __USB_PWR_H */
