#include "stm32f10x.h"
#include "i2c.h"

/**
 * @brief  Initializes I2C peripheral
 * @param  None
 * @retval None
 */
void sw_i2c_init(void)
{
	// GPIO clock enable
	RCC_APB2PeriphClockCmd(RCC_APB_GPIO, ENABLE);
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
	// GPIO Setting
	GPIO_InitTypeDef gpio_init_struct;
	gpio_init_struct.GPIO_Mode = GPIO_Mode_Out_OD;
	gpio_init_struct.GPIO_Speed = GPIO_Speed_50MHz;
	gpio_init_struct.GPIO_Pin = SCL_PIN | SDA_PIN;
	GPIO_Init(I2C_GPIOX, &gpio_init_struct);

	SW_I2C_SCL(Bit_SET);
	SW_I2C_SDA(Bit_SET);
}

/**
 * @brief  I2C start condition
 * @param  None
 * @retval None
 */
void sw_i2c_start(void)
{
	SW_I2C_SDA(Bit_SET);
	SW_I2C_SCL(Bit_SET);
	SW_I2C_SDA(Bit_RESET);
	SW_I2C_SCL(Bit_RESET);
}

/**
 * @brief  I2C stop condition
 * @param  None
 * @retval None
 */
void sw_i2c_stop(void)
{
	SW_I2C_SDA(Bit_RESET);
	SW_I2C_SCL(Bit_SET);
	SW_I2C_SDA(Bit_SET);
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
			SW_I2C_SDA(Bit_SET);
		}
		else
		{
			SW_I2C_SDA(Bit_RESET);
		}
		byte <<= 1;

		SW_I2C_SCL(Bit_SET);
		SW_I2C_SCL(Bit_RESET);
	}

	// receive ACK
	ack = SW_I2C_ReadBit();
	SW_I2C_SCL(Bit_SET);
	SW_I2C_SCL(Bit_RESET);
	return ack;
}

void hw_i2c_init(void)
{
	// GPIO clock enable
	RCC_APB2PeriphClockCmd(RCC_APB_GPIO, ENABLE);
	// GPIO Setting
	GPIO_InitTypeDef gpio_init_struct;
	gpio_init_struct.GPIO_Mode = GPIO_Mode_AF_OD;
	gpio_init_struct.GPIO_Pin = SCL_PIN | SDA_PIN;
	gpio_init_struct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(I2C_GPIOX, &gpio_init_struct);
	// I2C clock enable
	RCC_APB1PeriphClockCmd(RCC_APB_I2C, ENABLE);
	// I2C Setting
	I2C_InitTypeDef i2c_init_struct;
	i2c_init_struct.I2C_Mode = I2C_Mode_I2C;
	i2c_init_struct.I2C_ClockSpeed = I2C_FAST_SPEED;
	i2c_init_struct.I2C_DutyCycle = I2C_DutyCycle_2;
	i2c_init_struct.I2C_Ack = I2C_Ack_Disable;
	i2c_init_struct.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
	i2c_init_struct.I2C_OwnAddress1 = 0x00;
	I2C_Init(I2CX, &i2c_init_struct);

	I2C_Cmd(I2CX, ENABLE);
}
