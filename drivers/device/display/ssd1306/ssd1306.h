#ifndef _SSD1306_H
#define _SSD1306_H

#include "stm32f10x.h"
#include "sw_i2c.h"

void ssd1306_init(void);
void ssd1306_clear(void);
void ssd1306_show_char(uint8_t line, uint8_t column, uint8_t chr);
void ssd1306_show_string(uint8_t line, uint8_t column, uint8_t *str);
void ssd1306_show_num(uint8_t line, uint8_t column, uint32_t number, uint8_t length);
void ssd1306_show_signed_num(uint8_t line, uint8_t column, int32_t number,
		uint8_t length);
void ssd1306_show_hex_num(uint8_t line, uint8_t column, uint32_t number,
		uint8_t length);
void ssd1306_show_bin_num(uint8_t line, uint8_t column, uint32_t number,
		uint8_t length);

#endif
