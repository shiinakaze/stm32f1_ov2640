#include "stm32f10x.h"
#include "sw_i2c.h"

/**
 * @brief  Initializes software I2C pins
 * @param  None
 * @retval None
 */
void sw_i2c_init(void)
{
	// GPIO clock enable
	RCC_APB2PeriphClockCmd(SW_I2C_RCC_APB_GPIO, ENABLE);
	// 关闭 JTAG-DP 释放 PB3/PB4（保留 SWD）——DVP 控制引脚的前提
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
	// GPIO Setting
	GPIO_InitTypeDef gpio_init_struct;
	gpio_init_struct.GPIO_Mode = GPIO_Mode_Out_OD;
	gpio_init_struct.GPIO_Speed = GPIO_Speed_50MHz;
	gpio_init_struct.GPIO_Pin = SW_I2C_SCL_PIN | SW_I2C_SDA_PIN;
	GPIO_Init(SW_I2C_GPIO, &gpio_init_struct);

	SW_I2C_SCL_WRITE_BIT(Bit_SET);
	SW_I2C_SDA_WRITE_BIT(Bit_SET);
}

/**
 * @brief  I2C start condition
 * @param  None
 * @retval None
 */
void sw_i2c_start(void)
{
	SW_I2C_SDA_WRITE_BIT(Bit_SET);
	SW_I2C_SCL_WRITE_BIT(Bit_SET);
	SW_I2C_SDA_WRITE_BIT(Bit_RESET);
	SW_I2C_SCL_WRITE_BIT(Bit_RESET);
}

/**
 * @brief  I2C stop condition
 * @param  None
 * @retval None
 */
void sw_i2c_stop(void)
{
	SW_I2C_SDA_WRITE_BIT(Bit_RESET);
	SW_I2C_SCL_WRITE_BIT(Bit_SET);
	SW_I2C_SDA_WRITE_BIT(Bit_SET);
}

/**
 * @brief  Transmit one byte using I2C, ACK enable.
 * @param  Byte
 * @retval None
 */
uint8_t sw_i2c_transmit_byte(uint8_t byte)
{
	uint8_t ack;
	for (uint8_t i = 0; i < 8; i++)
	{
		if (byte & 0x80)
		{
			SW_I2C_SDA_WRITE_BIT(Bit_SET);
		}
		else
		{
			SW_I2C_SDA_WRITE_BIT(Bit_RESET);
		}
		byte <<= 1;

		SW_I2C_SCL_WRITE_BIT(Bit_SET);
		SW_I2C_SCL_WRITE_BIT(Bit_RESET);
	}

	// receive ACK
	ack = SW_I2C_SDA_READ_BIT();
	SW_I2C_SCL_WRITE_BIT(Bit_SET);
	SW_I2C_SCL_WRITE_BIT(Bit_RESET);
	return ack;
}
