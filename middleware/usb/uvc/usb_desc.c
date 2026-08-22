#include "usb_desc.h"

/* UVC 1.0 MJPEG 摄像头描述符。
 * 配置：接口 0 = VideoControl（无端点），
 *       接口 1 = VideoStreaming（alt0 零带宽 / alt1 ISO IN 368B），
 *       分辨率 320x240、160x120 与 640x360，帧率 30fps。 */

/* 设备描述符：0xEF/0x02/0x01 = Misc/IAD 复合设备约定 */
const uint8_t uvc_device_descriptor[UVC_SIZ_DEVICE_DESC] =
{
    0x12,                      /* bLength */
    0x01,                      /* bDescriptorType: DEVICE */
    0x00, 0x02,                /* bcdUSB 2.00 */
    0xEF,                      /* bDeviceClass: Misc */
    0x02,                      /* bDeviceSubClass: Common */
    0x01,                      /* bDeviceProtocol: IAD */
    0x40,                      /* bMaxPacketSize0 */
    0x34, 0x12,                /* idVendor (测试用) */
    0x78, 0x56,                /* idProduct (测试用) */
    0x00, 0x01,                /* bcdDevice 1.00 */
    0x01,                      /* iManufacturer */
    0x02,                      /* iProduct */
    0x03,                      /* iSerialNumber */
    0x01                       /* bNumConfigurations */
};

const uint8_t uvc_config_descriptor[UVC_SIZ_CONFIG_DESC] =
{
    /* 配置描述符 */
    0x09,                      /* bLength */
    0x02,                      /* bDescriptorType: CONFIGURATION */
    0xD3, 0x00,                /* wTotalLength: 211 */
    0x02,                      /* bNumInterfaces: 2 */
    0x01,                      /* bConfigurationValue */
    0x00,                      /* iConfiguration */
    0x80,                      /* bmAttributes: 总线供电 */
    0x32,                      /* bMaxPower: 100mA */

    /* 接口关联描述符（IAD）：VC + VS 组成视频功能 */
    0x08,                      /* bLength */
    0x0B,                      /* bDescriptorType: INTERFACE ASSOCIATION */
    0x00,                      /* bFirstInterface: 0 */
    0x02,                      /* bInterfaceCount: 2 */
    0x0E,                      /* bFunctionClass: Video */
    0x03,                      /* bFunctionSubClass: Video Interface Collection */
    0x00,                      /* bFunctionProtocol */
    0x02,                      /* iFunction */

    /* 接口 0：VideoControl，alt 0，无端点 */
    0x09,                      /* bLength */
    0x04,                      /* bDescriptorType: INTERFACE */
    0x00,                      /* bInterfaceNumber: 0 */
    0x00,                      /* bAlternateSetting: 0 */
    0x00,                      /* bNumEndpoints: 0 */
    0x0E,                      /* bInterfaceClass: Video */
    0x01,                      /* bInterfaceSubClass: Video Control */
    0x00,                      /* bInterfaceProtocol */
    0x00,                      /* iInterface */

    /* VC 类特定头描述符 */
    0x0D,                      /* bLength: 13 */
    0x24,                      /* bDescriptorType: CS_INTERFACE */
    0x01,                      /* bDescriptorSubtype: VC_HEADER */
    0x00, 0x01,                /* bcdUVC: 1.00 */
    0x27, 0x00,                /* wTotalLength: 13+17+9 = 39 */
    0x00, 0x6C, 0xDC, 0x02,    /* dwClockFrequency: 48MHz */
    0x01,                      /* bInCollection: 1 */
    0x01,                      /* baInterfaceNr(0): VS = 接口 1 */

    /* 输入端子（摄像头传感器） */
    0x11,                      /* bLength: 17 */
    0x24,                      /* bDescriptorType: CS_INTERFACE */
    0x02,                      /* bDescriptorSubtype: INPUT_TERMINAL */
    0x01,                      /* bTerminalID: 1 */
    0x01, 0x02,                /* wTerminalType: 0x0201 摄像头 */
    0x00,                      /* bAssocTerminal */
    0x00,                      /* iTerminal */
    0x00, 0x00,                /* wObjectiveFocalLengthMin */
    0x00, 0x00,                /* wObjectiveFocalLengthMax */
    0x00, 0x00,                /* wOcularFocalLength */
    0x02,                      /* bControlSize: 2 */
    0x00, 0x00,                /* bmControls: 无支持的控制 */

    /* 输出端子（USB 流） */
    0x09,                      /* bLength: 9 */
    0x24,                      /* bDescriptorType: CS_INTERFACE */
    0x03,                      /* bDescriptorSubtype: OUTPUT_TERMINAL */
    0x02,                      /* bTerminalID: 2 */
    0x01, 0x01,                /* wTerminalType: 0x0101 USB 流 */
    0x00,                      /* bAssocTerminal */
    0x01,                      /* bSourceID: 端子 1 */
    0x00,                      /* iTerminal */

    /* 接口 1：VideoStreaming，alt 0（零带宽，无端点） */
    0x09,                      /* bLength */
    0x04,                      /* bDescriptorType: INTERFACE */
    0x01,                      /* bInterfaceNumber: 1 */
    0x00,                      /* bAlternateSetting: 0 */
    0x00,                      /* bNumEndpoints: 0 */
    0x0E,                      /* bInterfaceClass: Video */
    0x02,                      /* bInterfaceSubClass: Video Streaming */
    0x00,                      /* bInterfaceProtocol */
    0x00,                      /* iInterface */

    /* VS 类特定输入头描述符 */
    0x0E,                      /* bLength: 14 */
    0x24,                      /* bDescriptorType: CS_INTERFACE */
    0x01,                      /* bDescriptorSubtype: VS_INPUT_HEADER */
    0x01,                      /* bNumFormats: 1 */
    0x79, 0x00,                /* wTotalLength: 14+11+30+30+30+6 = 121 */
    0x81,                      /* bEndPointAddress: EP1 IN */
    0x00,                      /* bmInfo: 不支持动态格式切换 */
    0x02,                      /* bTerminalLink: 输出端子 2 */
    0x00,                      /* bStillCaptureMethod: 不支持 */
    0x00,                      /* bTriggerSupport */
    0x00,                      /* bTriggerUsage */
    0x01,                      /* bControlSize: 1 */
    0x00,                      /* bmaControls(0): 无 VS 控制 */

    /* VS MJPEG 格式描述符 */
    0x0B,                      /* bLength: 11 */
    0x24,                      /* bDescriptorType: CS_INTERFACE */
    0x06,                      /* bDescriptorSubtype: VS_FORMAT_MJPEG */
    0x01,                      /* bFormatIndex: 1 */
    0x03,                      /* bNumFrameDescriptors: 3 */
    0x01,                      /* bmFlags: 固定大小样本 */
    0x01,                      /* bDefaultFrameIndex: 1 */
    0x00,                      /* bAspectRatioX */
    0x00,                      /* bAspectRatioY */
    0x00,                      /* bmInterlaceFlags: 逐行 */
    0x00,                      /* bCopyProtect */

    /* VS 帧描述符 1：320x240，30fps */
    0x1E,                      /* bLength: 26 + 4×1 = 30 */
    0x24,                      /* bDescriptorType: CS_INTERFACE */
    0x07,                      /* bDescriptorSubtype: VS_FRAME_MJPEG */
    0x01,                      /* bFrameIndex: 1 */
    0x00,                      /* bmCapabilities */
    0x40, 0x01,                /* wWidth: 320 */
    0xF0, 0x00,                /* wHeight: 240 */
    0x00, 0x80, 0x32, 0x02,    /* dwMinBitRate: 36.9Mbps */
    0x00, 0x80, 0x32, 0x02,    /* dwMaxBitRate: 36.9Mbps */
    0x00, 0x30, 0x00, 0x00,    /* dwMaxVideoFrameBufferSize: 12288 */
    0x15, 0x16, 0x05, 0x00,    /* dwDefaultFrameInterval: 333333 (30fps) */
    0x01,                      /* bFrameIntervalType: 1 个离散帧率 */
    0x15, 0x16, 0x05, 0x00,    /* dwFrameInterval(1): 333333 (30fps) */

    /* VS 帧描述符 2：160x120，30fps */
    0x1E,                      /* bLength: 26 + 4×1 = 30 */
    0x24,                      /* bDescriptorType: CS_INTERFACE */
    0x07,                      /* bDescriptorSubtype: VS_FRAME_MJPEG */
    0x02,                      /* bFrameIndex: 2 */
    0x00,                      /* bmCapabilities */
    0xA0, 0x00,                /* wWidth: 160 */
    0x78, 0x00,                /* wHeight: 120 */
    0x00, 0xA0, 0x8C, 0x00,    /* dwMinBitRate: 9.2Mbps */
    0x00, 0xA0, 0x8C, 0x00,    /* dwMaxBitRate: 9.2Mbps */
    0x00, 0x30, 0x00, 0x00,    /* dwMaxVideoFrameBufferSize: 12288 */
    0x15, 0x16, 0x05, 0x00,    /* dwDefaultFrameInterval: 333333 (30fps) */
    0x01,                      /* bFrameIntervalType: 1 个离散帧率 */
    0x15, 0x16, 0x05, 0x00,    /* dwFrameInterval(1): 333333 (30fps) */

    /* VS 帧描述符 3：640x360，30fps
     * （宽高比 16:9，驱动会在 640x480 输入窗口内垂直居中裁剪） */
    0x1E,                      /* bLength: 26 + 4×1 = 30 */
    0x24,                      /* bDescriptorType: CS_INTERFACE */
    0x07,                      /* bDescriptorSubtype: VS_FRAME_MJPEG */
    0x03,                      /* bFrameIndex: 3 */
    0x00,                      /* bmCapabilities */
    0x80, 0x02,                /* wWidth: 640 */
    0x68, 0x01,                /* wHeight: 360 */
    0x00, 0x80, 0x97, 0x06,    /* dwMinBitRate: 110.6Mbps */
    0x00, 0x80, 0x97, 0x06,    /* dwMaxBitRate: 110.6Mbps */
    0x00, 0x40, 0x00, 0x00,    /* dwMaxVideoFrameBufferSize: 16384 */
    0x15, 0x16, 0x05, 0x00,    /* dwDefaultFrameInterval: 333333 (30fps) */
    0x01,                      /* bFrameIntervalType: 1 个离散帧率 */
    0x15, 0x16, 0x05, 0x00,    /* dwFrameInterval(1): 333333 (30fps) */

    /* 颜色匹配描述符 */
    0x06,                      /* bLength: 6 */
    0x24,                      /* bDescriptorType: CS_INTERFACE */
    0x0D,                      /* bDescriptorSubtype: VS_COLORFORMAT */
    0x01,                      /* bColorPrimaries: BT.709/sRGB */
    0x01,                      /* bTransferCharacteristics: BT.709 */
    0x04,                      /* bMatrixCoefficients: SMPTE 170M */

    /* 接口 1：VideoStreaming，alt 1（工作态，ISO IN 端点） */
    0x09,                      /* bLength */
    0x04,                      /* bDescriptorType: INTERFACE */
    0x01,                      /* bInterfaceNumber: 1 */
    0x01,                      /* bAlternateSetting: 1 */
    0x01,                      /* bNumEndpoints: 1 */
    0x0E,                      /* bInterfaceClass: Video */
    0x02,                      /* bInterfaceSubClass: Video Streaming */
    0x00,                      /* bInterfaceProtocol */
    0x00,                      /* iInterface */

    /* ISO IN 端点描述符 */
    0x07,                      /* bLength */
    0x05,                      /* bDescriptorType: ENDPOINT */
    0x81,                      /* bEndpointAddress: EP1 IN */
    0x01,                      /* bmAttributes: 同步传输 */
    0xB8, 0x00,                /* wMaxPacketSize: 184 */
    0x01                       /* bInterval: 每帧 */
};

/* 字符串描述符 */
const uint8_t uvc_string_lang_id[UVC_SIZ_STRING_LANGID] =
{
    0x04,
    0x03,
    0x09, 0x04 /* 美式英语 */
};

const uint8_t uvc_string_vendor[UVC_SIZ_STRING_VENDOR] =
{
    0x0A, 0x03,
    'K', 0, 'a', 0, 'z', 0, 'e', 0
};

const uint8_t uvc_string_product[UVC_SIZ_STRING_PRODUCT] =
{
    0x26, 0x03,
    'S', 0, 'T', 0, 'M', 0, '3', 0, '2', 0, 'F', 0, '1', 0, ' ', 0,
    'O', 0, 'V', 0, '2', 0, '6', 0, '4', 0, '0', 0, ' ', 0,
    'U', 0, 'V', 0, 'C', 0
};

/* 序列号由 usb_bsp.c 用芯片唯一 ID 运行时填充 */
uint8_t uvc_string_serial[UVC_SIZ_STRING_SERIAL] =
{
    0x1A, 0x03,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
