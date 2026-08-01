#ifndef _SCCB_H
#define _SCCB_H

#include "main.h"
#include "delay.h"

#define RCC_APB_SCCB_GPIO RCC_APB2Periph_GPIOB
#define SCCB_GPIO GPIOB

#define SIO_C_PIN GPIO_Pin_10
#define SIO_D_PIN GPIO_Pin_11
#define SCCB_ERROR 0xFF

#define SW_SIO_C_Write(BIT_VALUE) GPIO_WriteBit(SCCB_GPIO, SIO_C_PIN, BIT_VALUE)
#define SW_SIO_D_Write(BIT_VALUE) GPIO_WriteBit(SCCB_GPIO, SIO_D_PIN, BIT_VALUE)
#define SW_SIO_D_Read() GPIO_ReadInputDataBit(SCCB_GPIO, SIO_D_PIN)
#define SW_SCCB_Delay() delay_us(5)

void sw_sccb_init(void);
void sw_sccb_write_reg(uint8_t id_address, uint8_t sub_address, uint8_t data);
uint8_t sw_sccb_read_reg(uint8_t id_address, uint8_t sub_address);

// void sccb_init(void);
// void sccb_write_reg(uint8_t id_address, uint8_t sub_address, uint8_t data);
// uint8_t sccb_read_reg(uint8_t id_address, uint8_t sub_address);
#endif // _SCCB_H
