/**
  ******************************************************************************
  * @file    sccb_bus.h 
  * @author  WangBao Dev Team
  * @version V1.0.0
  * @date    07-10-2012
  * @brief   Header for ov7670.c module
  ******************************************************************************
  * @attention
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2011 STMicroelectronics</center></h2>
  ******************************************************************************
  */ 
 
#ifndef _SCCB_BUS_H_
#define _SCCB_BUS_H_


#include "stm32f4xx.h"


#define SCCB_SIC_H()     GPIOF->BSRRL = GPIO_Pin_9
#define SCCB_SIC_L()     GPIOF->BSRRH = GPIO_Pin_9
#define SCCB_SID_H()     GPIOF->BSRRL = GPIO_Pin_8
#define SCCB_SID_L()     GPIOF->BSRRH = GPIO_Pin_8
#define SCCB_DATA_IN     sccb_bus_data_in()
#define SCCB_DATA_OUT    sccb_bus_data_out()
#define SCCB_SID_STATE	 GPIOF->IDR&0x0100

#define USE_RTOS
#define NOPNUM   14//168MHz          //16//180MHz

void systick_delay_ms(u32 nms);	    								   
void systick_delay_us(u32 nus);

void sccb_bus_init(void);
void sccb_bus_start(void);
void sccb_bus_stop(void);
void sccb_bus_send_noack(void);
void sccb_bus_send_ack(void);
uint8_t sccb_bus_write_byte(uint8_t data);
uint8_t sccb_bus_read_byte(void);


#endif /* _SCCB_BUS_H_ */

/******************* (C) COPYRIGHT 2012 WangBao *****END OF FILE****/
