#ifndef _SCCB_H
#define _SCCB_H

#include "main.h"
#include "delay.h"

/* SCCB 总线返回值 */
#define SCCB_OK 0
#define SCCB_FAIL 1

/* 引脚：PB10 = SIO_C（时钟），PB11 = SIO_D（数据） */
#define RCC_APB_SCCB_GPIO RCC_APB2Periph_GPIOB
#define SCCB_GPIO GPIOB

#define SIO_C_PIN GPIO_Pin_10
#define SIO_D_PIN GPIO_Pin_11

#define SW_SIO_C_Write(BIT_VALUE) GPIO_WriteBit(SCCB_GPIO, SIO_C_PIN, BIT_VALUE)
#define SW_SIO_D_Write(BIT_VALUE) GPIO_WriteBit(SCCB_GPIO, SIO_D_PIN, BIT_VALUE)
#define SW_SIO_D_Read() GPIO_ReadInputDataBit(SCCB_GPIO, SIO_D_PIN)

/* 位时序半周期 5us，时钟约 100kHz，低于 DS 表 9 规定的 400kHz 上限 */
#define SW_SCCB_Delay() delay_us(5)

void sw_sccb_init(void);
uint8_t sw_sccb_write_reg(uint8_t id_address, uint8_t sub_address, uint8_t data);
uint8_t sw_sccb_read_reg(uint8_t id_address, uint8_t sub_address, uint8_t *data);

#endif // _SCCB_H
