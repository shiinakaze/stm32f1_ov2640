#include "timer.h"

/* 系统时钟 72MHz（SystemInit：SYSCLK_FREQ_72MHz，UVC 需 48MHz USB 时钟）。
 * 注意：单次 delay_us 的 us 值不得超过 0xFFFFFF/72 ≈ 233016，
 * 当前调用方最大为 delay_ms 内部的 1000us。 */
#define MCU_SYSCLK 72

/**
  * @brief  Microsecond delay
  * @param  Delay:specifies the delay time length, in microseconds.
  * @retval 无
  */
void delay_us(uint32_t us)
{
	SysTick->LOAD = MCU_SYSCLK * us;			// Set systick reload value
	SysTick->VAL = 0x00;					// Set SysTick Current Value to 0
	SysTick->CTRL = 0x00000005;				// Set SysTick clock source to use processor clock and enable timer
	while(!(SysTick->CTRL & 0x00010000));	// Wait for the timer to count to 0
	SysTick->CTRL = 0x00000004;				// Disable timer
}

/**
  * @brief  Millisecond delay
  * @param  Delay: specifies the delay time length, in milliseconds.
  * @retval 无
  */
void delay_ms(uint32_t ms)
{
	while(ms--)
	{
		delay_us(1000);
	}
}

/**
  * @brief  Second delay
  * @param  Delay: specifies the delay time length, in seconds.
  * @retval 无
  */
void delay_s(uint32_t s)
{
	while(s--)
	{
		delay_ms(1000);
	}
}
