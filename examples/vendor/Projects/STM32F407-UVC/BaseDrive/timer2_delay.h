
#ifndef _TIMER2_DELAY_H
#define _TIMER2_DELAY_H

//#include "main.h"
#include "stm32f4xx.h"
void TIM2_delay_Init(void);
void delay_us(u32 us);
void delay_ms(u32 ms);

#endif
