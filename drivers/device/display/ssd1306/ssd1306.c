#include "oled.h"
#include "oled_font.h"
#include "delay.h"

// SSD1306 configuration
#define SSD1306_Clock RCC_APB2Periph_GPIOB
#define SSD1306_GPIOx GPIOB
#define SSD1306_SCL_Pin GPIO_Pin_4
#define SSD1306_SDA_Pin GPIO_Pin_5
#define SSD1306_Slave_Address 0x78
#define SSD1306_Control_Command 0x00
#define SSD1306_Control_Data 0x40

/**
 * @brief  	Write commands to the device using I2C
 * 			If you want to use hardware I2C, you can modify this function.
 * @param  	Command
 * @retval 	None
 */
void i2c_wc_ssd1306(uint8_t command)
{
	sw_i2c_start();
	sw_i2c_transmit_byte(SSD1306_Slave_Address);	  // slave address
	sw_i2c_transmit_byte(SSD1306_Control_Command); // write command
	sw_i2c_transmit_byte(command);
	sw_i2c_stop();
}

/**
 * @brief  	Write data to the device using I2C
 * 			If you want to use hardware I2C, you can modify this function.
 * @param 	Data
 * @retval 	None
 */
void i2c_wd_ssd1306(uint8_t data)
{
	// If you want to use hardware I2C, you can modify this function.
	sw_i2c_start();
	sw_i2c_transmit_byte(SSD1306_Slave_Address); // slave address
	sw_i2c_transmit_byte(SSD1306_Control_Data);	// write data
	sw_i2c_transmit_byte(data);
	sw_i2c_stop();
}

/**
 * @brief  OLED sets cursor position
 * @param  Y With the upper left corner as the origin, downward direction coordinates, range: 0-7
 * @param  X With the upper left corner as the origin, the coordinates in the right direction, range 0-127
 * @retval None
 */
void oled_set_cursor(uint8_t y, uint8_t x)
{
	i2c_wc_ssd1306(0xB0 | y);							  // Set Y position
	i2c_wc_ssd1306(0x10 | ((x & 0xF0) >> 4));			  // Set X position 4 bits higher
	i2c_wc_ssd1306(SSD1306_Control_Command | (x & 0x0F)); // Set X position 4 bits lower
}

/**
 * @brief  OLED clear screen
 * @param  None
 * @retval None
 */
void oled_clear(void)
{
	uint8_t i, j;
	for (j = 0; j < 8; j++)
	{
		oled_set_cursor(j, 0);
		for (i = 0; i < 128; i++)
		{
			i2c_wd_ssd1306(SSD1306_Control_Command);
		}
	}
}

/**
 * @brief  OLED displays a character
 * @param  Line Line position, value range 1-4
 * @param  Column Column position, value range 1-16
 * @param  Char A character to display, range: ASCII characters
 * @retval None
 */
void oled_show_char(uint8_t line, uint8_t column, uint8_t chr)
{
	uint8_t i;
	oled_set_cursor((line - 1) * 2, (column - 1) * 8); // Set the cursor position in the top half
	for (i = 0; i < 8; i++)
	{
		i2c_wd_ssd1306(font_8x16[chr - ' '][i]); // Displays the top half of the content
	}
	oled_set_cursor((line - 1) * 2 + 1, (column - 1) * 8); // Set the cursor position in the bottom half
	for (i = 0; i < 8; i++)
	{
		i2c_wd_ssd1306(font_8x16[chr - ' '][i + 8]); // Displays the bottom half of the content
	}
}

/**
 * @brief  OLED display string
 * @param  Line Line position, value range 1-4
 * @param  Column Column position, value range 1-16
 * @param  String String to display, range: ASCII characters
 * @retval None
 */
void oled_show_string(uint8_t line, uint8_t column, uint8_t *str)
{
	uint8_t i;
	for (i = 0; str[i] != '\0'; i++)
	{
		oled_show_char(line, column + i, str[i]);
	}
}

/**
 * @brief  OLED power function
 * @retval The return value is equal to X to the Y power
 */
uint32_t oled_pow(uint32_t x, uint32_t y)
{
	uint32_t result = 1;
	while (y--)
	{
		result *= x;
	}
	return result;
}

/**
 * @brief  OLED display numbers (decimal, positive)
 * @param  Line Line position, range 1-4
 * @param  Column Column position, range 1-16
 * @param  Number The number to display
 * @param  Length To display the length of the number, the value ranges from 1 to 10
 * @retval None
 */
void oled_show_num(uint8_t line, uint8_t column, uint32_t number, uint8_t length)
{
	uint8_t i;
	for (i = 0; i < length; i++)
	{
		oled_show_char(line, column + i,
					  number / oled_pow(10, length - i - 1) % 10 + '0');
	}
}

/**
 * @brief  OLED display number (decimal, signed number)
 * @param  Line Line position, range 1-4
 * @param  Column Column position, range 1-16
 * @param  Number The number to display
 * @param  Length To display the length of the number, the value ranges from 1 to 10
 * @retval None
 */
void oled_show_signed_num(uint8_t line, uint8_t column, int32_t number,
						uint8_t length)
{
	uint8_t i;
	uint32_t number1;
	if (number >= 0)
	{
		oled_show_char(line, column, '+');
		number1 = number;
	}
	else
	{
		oled_show_char(line, column, '-');
		number1 = -number;
	}
	for (i = 0; i < length; i++)
	{
		oled_show_char(line, column + i + 1,
					  number1 / oled_pow(10, length - i - 1) % 10 + '0');
	}
}

/**
 * @brief  OLED display numbers (hexadecimal, positive)
 * @param  Line Line position, range 1-4
 * @param  Column Column position, range 1-16
 * @param  Number The number to display， value range：0-0xFFFFFFFF
 * @param  Length To display the number length, the value ranges from 1 to 8
 * @retval None
 */
void oled_show_hex_num(uint8_t line, uint8_t column, uint32_t number,
					 uint8_t length)
{
	uint8_t i, single_number;
	for (i = 0; i < length; i++)
	{
		single_number = number / oled_pow(16, length - i - 1) % 16;
		if (single_number < 10)
		{
			oled_show_char(line, column + i, single_number + '0');
		}
		else
		{
			oled_show_char(line, column + i, single_number - 10 + 'A');
		}
	}
}

/**
 * @brief  OLED display numbers (binary, positive)
 * @param  Line Line position, range 1-4
 * @param  Column Column position, range 1-16
 * @param  Number The number to display，value range ：0-1111 1111 1111 1111
 * @param  Length To display the number length, the value ranges from 1 to 16
 * @retval None
 */
void oled_show_bin_num(uint8_t line, uint8_t column, uint32_t number,
					 uint8_t length)
{
	uint8_t i;
	for (i = 0; i < length; i++)
	{
		oled_show_char(line, column + i,
					  number / oled_pow(2, length - i - 1) % 2 + '0');
	}
}

/**
 * @brief  Initializes the OLED
 * @param  None
 * @retval None
 */
void oled_init(void)
{

	delay_s(1);
	sw_i2c_init();

	// software configuration from SSD1306 Application Note
	i2c_wc_ssd1306(0xA8); // Set MUX Ratio
	i2c_wc_ssd1306(0x3F);

	i2c_wc_ssd1306(0xD3); // Set Display Offset
	i2c_wc_ssd1306(0x00);

	i2c_wc_ssd1306(0x40); // Set Display Start Line

	i2c_wc_ssd1306(0xA1); // Set Segment re-map

	i2c_wc_ssd1306(0xC8); // Set COM Output Scan Direction

	i2c_wc_ssd1306(0xDA); // Set COM Pins hardware configuration
	i2c_wc_ssd1306(0x12);

	i2c_wc_ssd1306(0x81); // Set Contrast Control
	i2c_wc_ssd1306(0xCF);

	i2c_wc_ssd1306(0xA4); // Disable Entire Display On

	i2c_wc_ssd1306(0xA6); // Set Normal Display

	i2c_wc_ssd1306(0xD5); // Set Osc Frequency
	i2c_wc_ssd1306(0x80);

	i2c_wc_ssd1306(0x8D); // Enable charge pump regulator
	i2c_wc_ssd1306(0x14);

	i2c_wc_ssd1306(0xAF); // Display On AFh

	oled_clear(); // OLED Clear
}
