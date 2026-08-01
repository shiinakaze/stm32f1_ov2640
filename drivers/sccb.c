#include "sccb.h"

void sw_sccb_init(void)
{

	RCC_APB2PeriphClockCmd(RCC_APB_SCCB_GPIO, ENABLE);

	GPIO_InitTypeDef gpio_init_struct;
	gpio_init_struct.GPIO_Mode = GPIO_Mode_Out_PP;
	gpio_init_struct.GPIO_Speed = GPIO_Speed_50MHz;
	gpio_init_struct.GPIO_Pin = SIO_C_PIN | SIO_D_PIN;
	GPIO_Init(SCCB_GPIO, &gpio_init_struct);
}

void sw_sccb_sio_d_set_input(void)
{
	GPIO_InitTypeDef gpio_init_struct;
	gpio_init_struct.GPIO_Mode = GPIO_Mode_IPU;
	gpio_init_struct.GPIO_Pin = SIO_D_PIN;
	gpio_init_struct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(SCCB_GPIO, &gpio_init_struct);
}

void sw_sccb_sio_d_set_output(void)
{
	GPIO_InitTypeDef gpio_init_struct;
	gpio_init_struct.GPIO_Mode = GPIO_Mode_Out_PP;
	gpio_init_struct.GPIO_Pin = SIO_D_PIN;
	gpio_init_struct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(SCCB_GPIO, &gpio_init_struct);
}

void sw_sccb_start(void)
{
	// make sure SIO_C SIO_D high
	SW_SIO_C_Write(Bit_SET);
	SW_SIO_D_Write(Bit_SET);
	SW_SCCB_Delay();
	// SCCB Start condition
	SW_SIO_D_Write(Bit_RESET);
	SW_SCCB_Delay();
	SW_SIO_C_Write(Bit_RESET);
	SW_SCCB_Delay();
}

void sw_sccb_stop(void)
{
	// make sure SIO_D high
	SW_SIO_D_Write(Bit_RESET);
	SW_SCCB_Delay();
	// SCCB Stop condition
	SW_SIO_C_Write(Bit_SET);
	SW_SCCB_Delay();
	SW_SIO_D_Write(Bit_SET);
	SW_SCCB_Delay();
}

uint8_t sw_sccb_write_byte(uint8_t byte)
{
	uint8_t x_bit;
	for (uint8_t i = 0; i < 8; i++)
	{
		if (byte & 0x80)
		{
			SW_SIO_D_Write(Bit_SET);
		}
		else
		{
			SW_SIO_D_Write(Bit_RESET);
		}
		byte <<= 1;
		SW_SCCB_Delay();
		SW_SIO_C_Write(Bit_SET);
		SW_SCCB_Delay();
		SW_SIO_C_Write(Bit_RESET);
		SW_SCCB_Delay();
	}
	sw_sccb_sio_d_set_input();
	SW_SCCB_Delay();
	SW_SIO_C_Write(Bit_SET);
	x_bit = SW_SIO_D_Read(); // X/Don't care bit, typical value is 0
	SW_SCCB_Delay();
	SW_SIO_C_Write(Bit_RESET);
	SW_SCCB_Delay();
	sw_sccb_sio_d_set_output();

	// check sccb data
	if (x_bit != 0)
	{
		return SCCB_ERROR;
	}
	return x_bit;
}

uint8_t sw_sccb_read_byte(void)
{
	uint8_t na_bit;
	sw_sccb_sio_d_set_input();
	uint8_t byte = 0x00;
	for (uint8_t i = 0; i < 8; i++)
	{
		SW_SCCB_Delay();
		SW_SIO_C_Write(Bit_SET);
		if (SW_SIO_D_Read())
		{
			byte |= (0x80 >> i);
		}
		SW_SCCB_Delay();
		SW_SIO_C_Write(Bit_RESET);
		SW_SCCB_Delay();
	}
	SW_SCCB_Delay();
	SW_SIO_C_Write(Bit_SET);
	na_bit = SW_SIO_D_Read(); // NA bit, typical value is 1
	SW_SCCB_Delay();
	SW_SIO_C_Write(Bit_RESET);
	SW_SCCB_Delay();
	sw_sccb_sio_d_set_output();

	// check sccb data
	if (na_bit != 1)
	{
		return SCCB_ERROR;
	}
	return byte;
}

void sw_sccb_write_reg(uint8_t id_address, uint8_t sub_address, uint8_t data)
{
	// 3-Phase Write
	sw_sccb_start();
	sw_sccb_write_byte(id_address);
	sw_sccb_write_byte(sub_address);
	sw_sccb_write_byte(data);
	sw_sccb_stop();
}

uint8_t sw_sccb_read_reg(uint8_t id_address, uint8_t sub_address)
{
	uint8_t data;
	// 2-Phase Write
	sw_sccb_start();
	sw_sccb_write_byte(id_address);
	sw_sccb_write_byte(sub_address);
	sw_sccb_stop();
	// 2-Phase Read
	sw_sccb_start();
	sw_sccb_write_byte(id_address | 0x01);
	data = sw_sccb_read_byte();
	sw_sccb_stop();

	return data;
}

void sccb_init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB_SCCB_GPIO, ENABLE);

	GPIO_InitTypeDef gpio_init_struct;
	gpio_init_struct.GPIO_Mode = GPIO_Mode_AF_OD;
	gpio_init_struct.GPIO_Speed = GPIO_Speed_50MHz;
	gpio_init_struct.GPIO_Pin = SIO_C_PIN | SIO_D_PIN;
	GPIO_Init(SCCB_GPIO, &gpio_init_struct);

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C2, ENABLE);

	I2C_InitTypeDef i2c_init_struct;
	i2c_init_struct.I2C_ClockSpeed = 400000;
	i2c_init_struct.I2C_Mode = I2C_Mode_I2C;
	i2c_init_struct.I2C_DutyCycle = I2C_DutyCycle_2;
	i2c_init_struct.I2C_OwnAddress1 = 0x00;
	i2c_init_struct.I2C_Ack = I2C_Ack_Enable;
	i2c_init_struct.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;

	I2C_Init(I2C2, &i2c_init_struct);
	I2C_Cmd(I2C2, ENABLE);
}
