#include "main.h"
#include "ov2640.h"
#include "oled.h"
#include "delay.h"
#include "uart.h"
#include <stdio.h>

int main(void)
{
    OLED_Init();
    UART1_Init(115200);

    OV2640_Init();
    OV2640_TimerMeasure_Init();

    OLED_ShowHexNum(1, 1, OV2640_GetPID(), 4);
    OLED_ShowHexNum(2, 1, OV2640_GetMID(), 4);

    while (1)
    {
        OV2640_SignalMeasure_t result;

        OV2640_Test_FrameSignal(&result);

        printf("VSYNC_H: %lu us\r\n", result.vsync_high_us);
        printf("HREF_C : %lu\r\n", result.href_count);
        printf("PCLK_C : %lu\r\n", result.pclk_count);
        printf("HREF_F : %.2f Hz\r\n", result.href_freq);
        printf("PCLK_F : %.2f Hz\r\n", result.pclk_freq);
        printf("--------------------\r\n");

        Delay_ms(1000);
    }
}