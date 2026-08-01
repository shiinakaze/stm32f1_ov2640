#include "main.h"
#include "ov2640.h"
#include "oled.h"
#include "delay.h"
#include "uart.h"

int main(void)
{
    uint8_t *frame;
    uint32_t len;

    OLED_Init();
    UART1_Init(1500000);
    OV2640_Init();
    OLED_ShowHexNum(1, 1, OV2640_GetPID(), 4);
    OLED_ShowHexNum(2, 1, OV2640_GetMID(), 4);

    while (1)
    {
        /* 采集一帧到当前空闲缓冲（与上一次 DMA 发送并行） */
        OV2640_Capture();

        /* 等待上一帧 DMA 发送完毕，再启动新帧发送 */
        while (!UART1_IsTransmitComplete())
        {
        }

        frame = OV2640_GetReadyFrame(&len);
        if (frame != NULL && len > 0)
        {
            /* 非阻塞启动 DMA 发送；下一轮采集会写入另一块缓冲 */
            UART1_Transmit_NonBlocking(frame, (uint16_t)len);
        }
    }
}
