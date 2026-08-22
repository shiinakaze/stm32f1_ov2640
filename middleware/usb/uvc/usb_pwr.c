#include "usb_lib.h"
#include "usb_conf.h"
#include "usb_pwr.h"
#include "usbd_uvc.h"

/* 简化的电源管理：只处理挂起标志与 USB 外设 FSUSP 状态，
 * 不做时钟门控/低功耗休眠（系统保持 72MHz 运行，相机随时可采集）。 */

__IO uint32_t bDeviceState = UNCONNECTED;
__IO uint32_t fSuspendEnabled = 1;

RESULT PowerOn(void)
{
    uint16_t cntr;

    /* 强制复位 USB 外设并释放，清空中断状态后打开事件屏蔽 */
    cntr = CNTR_FRES;
    _SetCNTR(cntr);

    wInterrupt_Mask = 0;
    _SetCNTR(wInterrupt_Mask);

    _SetISTR(0);

    wInterrupt_Mask = CNTR_RESETM | CNTR_SUSPM | CNTR_WKUPM |
                      CNTR_ERRM | CNTR_SOFM | CNTR_ESOFM | CNTR_CTRM;
    _SetCNTR(wInterrupt_Mask);

    return USB_SUCCESS;
}

RESULT PowerOff(void)
{
    _SetCNTR(CNTR_FRES | CNTR_PDWN);
    return USB_SUCCESS;
}

void Suspend(void)
{
    uint16_t cntr = _GetCNTR();

    cntr |= CNTR_FSUSP;
    _SetCNTR(cntr);

    uvc_stream_disable();

    bDeviceState = SUSPENDED;
}

void Resume(RESUME_STATE e_resume_set_val)
{
    uint16_t cntr;

    (void)e_resume_set_val;

    if (bDeviceState == SUSPENDED)
    {
        cntr = _GetCNTR();
        cntr &= ~(CNTR_FSUSP | CNTR_LPMODE | CNTR_RESUME);
        _SetCNTR(cntr);

        bDeviceState = (pInformation->Current_Configuration != 0)
                           ? CONFIGURED
                           : ATTACHED;
    }
}
