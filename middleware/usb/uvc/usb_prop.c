#include "usb_lib.h"
#include "usb_conf.h"
#include "usb_prop.h"
#include "usb_pwr.h"
#include "usb_bsp.h"
#include "usb_desc.h"
#include "usbd_uvc.h"

/* UVC 类特定请求码（usb_def.h 中 GET_CUR 等已有定义，此处补充） */
#define UVC_GET_LEN (0x85)
#define UVC_GET_INFO (0x86)

static void uvc_user_set_configuration(void);
static void uvc_user_set_interface(void);
static void uvc_user_set_address(void);

DEVICE Device_Table =
{
    EP_NUM,
    1
};

DEVICE_PROP Device_Property =
{
    uvc_device_init,
    uvc_device_reset,
    uvc_device_status_in,
    uvc_device_status_out,
    uvc_device_data_setup,
    uvc_device_no_data_setup,
    uvc_device_get_interface_setting,
    uvc_device_get_device_descriptor,
    uvc_device_get_config_descriptor,
    uvc_device_get_string_descriptor,
    0,
    0x40 /* EP0 最大包长 */
};

USER_STANDARD_REQUESTS User_Standard_Requests =
{
    0,                          /* User_GetConfiguration */
    uvc_user_set_configuration, /* User_SetConfiguration */
    0,                          /* User_GetInterface */
    uvc_user_set_interface,     /* User_SetInterface */
    0,                          /* User_GetStatus */
    0,                          /* User_ClearFeature */
    0,                          /* User_SetEndPointFeature */
    0,                          /* User_SetDeviceFeature */
    uvc_user_set_address        /* User_SetDeviceAddress */
};

static ONE_DESCRIPTOR device_descriptor =
{
    (uint8_t *)uvc_device_descriptor,
    UVC_SIZ_DEVICE_DESC
};

static ONE_DESCRIPTOR config_descriptor =
{
    (uint8_t *)uvc_config_descriptor,
    UVC_SIZ_CONFIG_DESC
};

static ONE_DESCRIPTOR string_descriptors[4] =
{
    {(uint8_t *)uvc_string_lang_id, UVC_SIZ_STRING_LANGID},
    {(uint8_t *)uvc_string_vendor, UVC_SIZ_STRING_VENDOR},
    {(uint8_t *)uvc_string_product, UVC_SIZ_STRING_PRODUCT},
    {(uint8_t *)uvc_string_serial, UVC_SIZ_STRING_SERIAL},
};

void uvc_device_init(void)
{
    uvc_get_serial_num();
    pInformation->Current_Configuration = 0;

    PowerOn();
    USB_SIL_Init();

    bDeviceState = UNCONNECTED;
}

void uvc_device_reset(void)
{
    pInformation->Current_Configuration = 0;
    pInformation->Current_Interface = 0;
    pInformation->Current_AlternateSetting = 0;

    /* bmAttributes + bMaxPower 组成的当前配置特征值 */
    pInformation->Current_Feature = uvc_config_descriptor[7];

    uvc_control_init();
    uvc_stream_disable();

    SetBTABLE(BTABLE_ADDRESS);

    /* EP0 控制端点 */
    SetEPType(ENDP0, EP_CONTROL);
    SetEPTxStatus(ENDP0, EP_TX_NAK);
    SetEPRxAddr(ENDP0, ENDP0_RXADDR);
    SetEPRxCount(ENDP0, Device_Property.MaxPacketSize);
    SetEPTxAddr(ENDP0, ENDP0_TXADDR);
    Clear_Status_Out(ENDP0);
    SetEPRxValid(ENDP0);

    /* EP1 ISO IN（硬件强制双缓冲，RM0008 §23.4.4）：
     * BUF0/BUF1 分别映射 TX/RX 两组描述符，复位后保持禁用 */
    SetEPType(ENDP1, EP_ISOCHRONOUS);
    SetEPDblBuffAddr(ENDP1, ENDP1_BUF0ADDR, ENDP1_BUF1ADDR);
    ClearDTOG_RX(ENDP1);
    ClearDTOG_TX(ENDP1);
    SetEPTxStatus(ENDP1, EP_TX_DIS);

    SetDeviceAddress(0);

    bDeviceState = ATTACHED;
}

void uvc_device_status_in(void)
{
}

void uvc_device_status_out(void)
{
}

/*---------------- 类特定请求（probe / commit） ----------------*/

static uint8_t status_buf[2];

/* CopyRoutine 模式：Length==0 时回填数据总长，否则返回缓冲区内偏移 */
static uint8_t *probe_command(uint16_t length)
{
    if (length == 0)
    {
        pInformation->Ctrl_Info.Usb_wLength = UVC_PROBE_COMMIT_LEN;
        return NULL;
    }
    return &uvc_probe_data[length];
}

static uint8_t *commit_command(uint16_t length)
{
    if (length == 0)
    {
        pInformation->Ctrl_Info.Usb_wLength = UVC_PROBE_COMMIT_LEN;
        return NULL;
    }
    return &uvc_commit_data[length];
}

static uint8_t *get_info_command(uint16_t length)
{
    if (length == 0)
    {
        pInformation->Ctrl_Info.Usb_wLength = 1;
        return NULL;
    }
    return &status_buf[0];
}

static uint8_t *get_len_command(uint16_t length)
{
    if (length == 0)
    {
        pInformation->Ctrl_Info.Usb_wLength = 2;
        return NULL;
    }
    return &status_buf[0];
}

RESULT uvc_device_data_setup(uint8_t request_no)
{
    uint8_t *(*copy_routine)(uint16_t);

    copy_routine = NULL;

    /* 仅接受以 VS 接口（接口 1）为接收者的类请求 */
    if ((Type_Recipient != (CLASS_REQUEST | INTERFACE_RECIPIENT)) ||
        (pInformation->USBwIndex0 != 1) || (pInformation->USBwIndex1 != 0))
    {
        return USB_UNSUPPORT;
    }

    status_buf[0] = 0x03; /* GET_INFO：支持 GET 与 SET */
    status_buf[1] = 0;

    switch (request_no)
    {
    case GET_CUR:
    case GET_DEF:
    case GET_MIN:
    case GET_MAX:
        if (pInformation->USBwValue1 == 0x01)      /* PROBE */
        {
            copy_routine = probe_command;
        }
        else if (pInformation->USBwValue1 == 0x02) /* COMMIT */
        {
            copy_routine = commit_command;
        }
        break;

    case SET_CUR:
        if (pInformation->USBwValue1 == 0x01)      /* PROBE */
        {
            copy_routine = probe_command;
        }
        else if (pInformation->USBwValue1 == 0x02) /* COMMIT */
        {
            copy_routine = commit_command;
        }
        break;

    case UVC_GET_INFO:
        copy_routine = get_info_command;
        break;

    case UVC_GET_LEN:
        status_buf[0] = UVC_PROBE_COMMIT_LEN & 0xFF;
        status_buf[1] = 0;
        copy_routine = get_len_command;
        break;

    default:
        return USB_UNSUPPORT;
    }

    if (copy_routine == NULL)
    {
        return USB_UNSUPPORT;
    }

    pInformation->Ctrl_Info.CopyData = copy_routine;
    pInformation->Ctrl_Info.Usb_wOffset = 0;
    (*copy_routine)(0);

    return USB_SUCCESS;
}

RESULT uvc_device_no_data_setup(uint8_t request_no)
{
    (void)request_no;
    return USB_UNSUPPORT;
}

RESULT uvc_device_get_interface_setting(uint8_t interface, uint8_t alternate)
{
    if (interface == 0)
    {
        return (alternate == 0) ? USB_SUCCESS : USB_UNSUPPORT;
    }
    if (interface == 1)
    {
        return (alternate <= 1) ? USB_SUCCESS : USB_UNSUPPORT;
    }
    return USB_UNSUPPORT;
}

uint8_t *uvc_device_get_device_descriptor(uint16_t length)
{
    return Standard_GetDescriptorData(length, &device_descriptor);
}

uint8_t *uvc_device_get_config_descriptor(uint16_t length)
{
    return Standard_GetDescriptorData(length, &config_descriptor);
}

uint8_t *uvc_device_get_string_descriptor(uint16_t length)
{
    uint8_t string_index = pInformation->USBwValue0;

    if (string_index >= 4)
    {
        return NULL;
    }
    return Standard_GetDescriptorData(length, &string_descriptors[string_index]);
}

/*---------------- 标准请求用户回调 ----------------*/

static void uvc_user_set_configuration(void)
{
    if (pInformation->Current_Configuration != 0)
    {
        bDeviceState = CONFIGURED;
    }
}

/* SET_INTERFACE：主机通过 alt 1 / alt 0 启停视频流 */
static void uvc_user_set_interface(void)
{
    if (pInformation->USBwIndex0 == 1)
    {
        if (pInformation->USBwValue0 == 1)
        {
            uvc_stream_enable();
        }
        else
        {
            uvc_stream_disable();
        }
    }
}

static void uvc_user_set_address(void)
{
    bDeviceState = ADDRESSED;
}
