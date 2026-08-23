#ifndef _UART_H
#define _UART_H

#include "app.h"
#include "board_config.h"
#include <stdio.h>
#include <string.h>

/* 引脚/外设选择（USART1、DMA1_Channel4）见 boards/bluepill/board_config.h */

void uart1_init(uint32_t baudrate);
void uart1_dma_init(void);
void uart1_transmit(uint8_t *tx_buffer, uint16_t tx_length);
void uart1_transmit_non_blocking(uint8_t *tx_buffer, uint16_t tx_length);
uint8_t uart1_is_transmit_complete(void);
void uart1_send_string(char *str);
void uart_receive(uint8_t *rx_buffer, uint16_t rx_length);

#endif
