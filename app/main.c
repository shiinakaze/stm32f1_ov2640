#include "main.h"
#include "ov2640.h"
#include "oled.h"
#include "delay.h"
#include "uart.h"

extern uint32_t SystemCoreClock;

/* 双缓冲帧存储：由上层定义，注入 OV2640 驱动。
 * 每块 8KB，共 16KB（STM32F103C8 的 20KB SRAM 可容纳）。 */
#define JPEG_BUFFER_SIZE (8 * 1024)
static uint8_t jpeg_buffer[2][JPEG_BUFFER_SIZE];

/* 运行时超频到 128MHz（HSE 8MHz × PLL16）。
 * SystemInit 已将时钟设为 72MHz，此处切换到 128MHz。
 * AHB=128MHz, APB1=32MHz(/4), APB2=64MHz(/2), Flash 2WS+预取。
 * 超出 STM32F103 规格（额定 72MHz），稳定性不保证；
 * 若不稳定请注释掉 main 中的 sys_clock_set_to_128() 调用。 */
static void sys_clock_set_to_128(void)
{
    /* 1. 切换系统时钟到 HSE，脱离 PLL（PLL 运行时不能改倍频） */
    RCC->CFGR = (RCC->CFGR & ~(uint32_t)RCC_CFGR_SW) | (uint32_t)RCC_CFGR_SW_HSE;
    while ((RCC->CFGR & (uint32_t)RCC_CFGR_SWS) != (uint32_t)RCC_CFGR_SWS_HSE)
    {
    }

    /* 2. 关闭 PLL */
    RCC->CR &= ~(uint32_t)RCC_CR_PLLON;

    /* 3. Flash 2 等待周期 + 预取（72MHz 时已是 2 WS，此处确保） */
    FLASH->ACR |= FLASH_ACR_PRFTBE;
    FLASH->ACR = (FLASH->ACR & ~(uint32_t)FLASH_ACR_LATENCY) | (uint32_t)FLASH_ACR_LATENCY_2;

    /* 4. 总线分频：先清零各字段再设置（SystemInit 已写过 72MHz 的值，必须清除） */
    RCC->CFGR &= ~(uint32_t)(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2);
    RCC->CFGR |= (uint32_t)(RCC_CFGR_HPRE_DIV1   |  /* HCLK  = 128MHz */
                             RCC_CFGR_PPRE2_DIV2  |  /* PCLK2 = 64MHz  */
                             RCC_CFGR_PPRE1_DIV4);   /* PCLK1 = 32MHz  */

    /* 5. PLL: HSE × 16 = 128MHz */
    RCC->CFGR &= ~(uint32_t)(RCC_CFGR_PLLSRC | RCC_CFGR_PLLXTPRE | RCC_CFGR_PLLMULL);
    RCC->CFGR |= (uint32_t)(RCC_CFGR_PLLSRC_HSE | RCC_CFGR_PLLMULL16);

    /* 6. 开启 PLL 并等待就绪 */
    RCC->CR |= RCC_CR_PLLON;
    while ((RCC->CR & RCC_CR_PLLRDY) == 0)
    {
    }

    /* 7. 切换系统时钟到 PLL，等待切换完成 */
    RCC->CFGR = (RCC->CFGR & ~(uint32_t)RCC_CFGR_SW) | (uint32_t)RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & (uint32_t)RCC_CFGR_SWS) != (uint32_t)RCC_CFGR_SWS_PLL)
    {
    }

    SystemCoreClock = 128000000;
}

int main(void)
{
    uint8_t *frame;
    uint32_t len;

    sys_clock_set_to_128();

    oled_init();
    uart1_init(1500000);
    ov2640_init();
    ov2640_set_frame_buffer(jpeg_buffer[0], jpeg_buffer[1], JPEG_BUFFER_SIZE);
    oled_show_hex_num(1, 1, ov2640_get_pid(), 4);
    oled_show_hex_num(2, 1, ov2640_get_mid(), 4);

    while (1)
    {
        /* 采集一帧到当前空闲缓冲（与上一次 DMA 发送并行） */
        ov2640_capture();

        /* 等待上一帧 DMA 发送完毕，再启动新帧发送 */
        while (!uart1_is_transmit_complete())
        {
        }

        frame = ov2640_get_ready_frame(&len);
        if (frame != NULL && len > 0)
        {
            /* 非阻塞启动 DMA 发送；下一轮采集会写入另一块缓冲 */
            uart1_transmit_non_blocking(frame, (uint16_t)len);
        }
    }
}
