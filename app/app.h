#ifndef _APP_H
#define _APP_H

/* 传输通道选择（编译期切换，改后重新编译）：
 *   APP_TRANSPORT_UVC  = USB 视频类设备（Windows/Linux 直接识别为摄像头）
 *   APP_TRANSPORT_UART = UART 传输（1.5M 波特率，配合 tools/main.py 接收，调试用）
 */
#define APP_TRANSPORT_UVC (1)
#define APP_TRANSPORT_UART (2)

#ifndef APP_TRANSPORT
#define APP_TRANSPORT APP_TRANSPORT_UVC
#endif

#include "stm32f10x.h"

/* 应用入口：由 kernel/baremetal/main.c 的 main() 调用，不返回 */
void app_main(void);

#endif // _APP_H
