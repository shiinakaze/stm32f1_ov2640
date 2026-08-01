#ifndef _OLED_H
#define _OLED_H

#include "main.h"
#include "i2c.h"

void oled_init(void);
void oled_clear(void);
void oled_show_char(uint8_t line, uint8_t column, uint8_t chr);
void oled_show_string(uint8_t line, uint8_t column, uint8_t *str);
void oled_show_num(uint8_t line, uint8_t column, uint32_t number, uint8_t length);
void oled_show_signed_num(uint8_t line, uint8_t column, int32_t number,
		uint8_t length);
void oled_show_hex_num(uint8_t line, uint8_t column, uint32_t number,
		uint8_t length);
void oled_show_bin_num(uint8_t line, uint8_t column, uint32_t number,
		uint8_t length);

#endif
