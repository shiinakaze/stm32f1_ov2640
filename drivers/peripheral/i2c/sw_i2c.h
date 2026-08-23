#ifndef _I2C_H
#define _I2C_H

#include "stm32f10x.h"

/*Configuration*/

/*I2C*/
#define I2CX I2C1
#define RCC_APB_I2C RCC_APB1Periph_I2C1
/*GPIO*/
#define RCC_APB_GPIO (RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO)
#define I2C_GPIOX GPIOB
/*Pin*/
#define SCL_PIN GPIO_Pin_6
#define SDA_PIN GPIO_Pin_7
/*Other*/
#define I2C_STD_SPEED 100000
#define I2C_FAST_SPEED 400000
#define SW_I2C_ACK 0
#define SW_I2C_NONE_ACK 1

#define SW_I2C_SCL(BIT_VALUE) GPIO_WriteBit(I2C_GPIOX, SCL_PIN, BIT_VALUE)
#define SW_I2C_SDA(BIT_VALUE) GPIO_WriteBit(I2C_GPIOX, SDA_PIN, BIT_VALUE)
#define SW_I2C_ReadBit() GPIO_ReadInputDataBit(I2C_GPIOX, SDA_PIN)

/*Software I2C*/
void sw_i2c_init(void);
void sw_i2c_start(void);
void sw_i2c_stop(void);
uint8_t sw_i2c_transmit_byte(uint8_t byte);

/*Hardware I2C*/
void hw_i2c_init(void);

#endif // _I2C_H
