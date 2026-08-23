
#ifndef __SRAM_H
#define __SRAM_H	

//#include "main.h" 
#include "stm32f4xx.h"
#define Bank1_SRAM3_ADDR    ((u32)(0x68000000))	
											  
void SRAM_Init(void);        //初始化控制SRAM的FSMC

#endif

