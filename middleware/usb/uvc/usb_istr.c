#include "usb_lib.h"
#include "usb_conf.h"
#include "usb_prop.h"
#include "usb_pwr.h"
#include "usb_istr.h"
#include "usbd_uvc.h"

__IO uint16_t wIstr;          /* ISTR 寄存器最近一次读取值 */
__IO uint32_t esof_counter = 0;
__IO uint32_t wCNTR = 0;

/* 非控制端点的回调表（usb_conf.h 中全部映射为 NOP_Process） */
void (*pEpInt_IN[7])(void) =
{
    EP1_IN_Callback,
    EP2_IN_Callback,
    EP3_IN_Callback,
    EP4_IN_Callback,
    EP5_IN_Callback,
    EP6_IN_Callback,
    EP7_IN_Callback,
};

void (*pEpInt_OUT[7])(void) =
{
    EP1_OUT_Callback,
    EP2_OUT_Callback,
    EP3_OUT_Callback,
    EP4_OUT_Callback,
    EP5_OUT_Callback,
    EP6_OUT_Callback,
    EP7_OUT_Callback,
};

void USB_Istr(void)
{
    __IO uint32_t ep[8];
    uint32_t i;

    wIstr = _GetISTR();

#if (IMR_MSK & ISTR_CTR)
    if (wIstr & ISTR_CTR & wInterrupt_Mask)
    {
        CTR_LP();
    }
#endif

#if (IMR_MSK & ISTR_RESET)
    if (wIstr & ISTR_RESET & wInterrupt_Mask)
    {
        _SetISTR((uint16_t)CLR_RESET);
        Device_Property.Reset();
    }
#endif

#if (IMR_MSK & ISTR_DOVR)
    if (wIstr & ISTR_DOVR & wInterrupt_Mask)
    {
        _SetISTR((uint16_t)CLR_DOVR);
    }
#endif

#if (IMR_MSK & ISTR_ERR)
    if (wIstr & ISTR_ERR & wInterrupt_Mask)
    {
        _SetISTR((uint16_t)CLR_ERR);
    }
#endif

#if (IMR_MSK & ISTR_WKUP)
    if (wIstr & ISTR_WKUP & wInterrupt_Mask)
    {
        _SetISTR((uint16_t)CLR_WKUP);
        Resume(RESUME_EXTERNAL);
    }
#endif

#if (IMR_MSK & ISTR_SUSP)
    if (wIstr & ISTR_SUSP & wInterrupt_Mask)
    {
        if (fSuspendEnabled)
        {
            Suspend();
        }
        else
        {
            Resume(RESUME_LATER);
        }
        _SetISTR((uint16_t)CLR_SUSP);
    }
#endif

#if (IMR_MSK & ISTR_SOF)
    if (wIstr & ISTR_SOF & wInterrupt_Mask)
    {
        _SetISTR((uint16_t)CLR_SOF);
    }
#endif

#if (IMR_MSK & ISTR_ESOF)
    if (wIstr & ISTR_ESOF & wInterrupt_Mask)
    {
        _SetISTR((uint16_t)CLR_ESOF);

        /* 连续 3 次 ESOF 期间 D+ 仍为高且未进入 FSUSP：
         * 可能丢失了 SUSP 标志，强制产生一次复位恢复 */
        if ((_GetFNR() & FNR_RXDP) != 0)
        {
            esof_counter++;
            if ((esof_counter > 3) && ((_GetCNTR() & CNTR_FSUSP) == 0))
            {
                wCNTR = _GetCNTR();
                for (i = 0; i < 8; i++)
                {
                    ep[i] = _GetENDPOINT(i);
                }

                wCNTR |= CNTR_FRES;
                _SetCNTR(wCNTR);
                wCNTR &= ~CNTR_FRES;
                _SetCNTR(wCNTR);

                while ((_GetISTR() & ISTR_RESET) == 0)
                {
                }
                _SetISTR((uint16_t)CLR_RESET);

                for (i = 0; i < 8; i++)
                {
                    _SetENDPOINT(i, (uint16_t)ep[i]);
                }

                esof_counter = 0;
            }
        }
        else
        {
            esof_counter = 0;
        }
    }
#endif
}
