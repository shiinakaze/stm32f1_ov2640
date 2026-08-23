#ifndef _SW_I2C_H
#define _SW_I2C_H

#include "stm32f10x.h"
#include "board_config.h"

/* 引脚/时钟选择（PB6/PB7 → OLED）见 boards/bluepill/board_config.h */

#define SW_I2C_ACK 0
#define SW_I2C_NONE_ACK 1

#define SW_I2C_SCL_WRITE_BIT(bit_val) GPIO_WriteBit(SW_I2C_GPIO, SW_I2C_SCL_PIN, (BitAction)(bit_val))
#define SW_I2C_SDA_WRITE_BIT(bit_val) GPIO_WriteBit(SW_I2C_GPIO, SW_I2C_SDA_PIN, (BitAction)(bit_val))
#define SW_I2C_SDA_READ_BIT() GPIO_ReadInputDataBit(SW_I2C_GPIO, SW_I2C_SDA_PIN)

void sw_i2c_init(void);
void sw_i2c_start(void);
void sw_i2c_stop(void);
uint8_t sw_i2c_transmit_byte(uint8_t byte);

#endif // _SW_I2C_H
