#ifndef _UART_H
#define _UART_H

#include "main.h"
#include <stdio.h>
#include <string.h>

#define RCC_APB_UART1_GPIO      RCC_APB2Periph_GPIOA
#define RCC_APB_UART1           RCC_APB2Periph_USART1
#define RCC_AHB_UART1_DMA       RCC_AHBPeriph_DMA1

#define UART1_GPIO              GPIOA
#define UART1_TX_PIN            GPIO_Pin_9
#define UART1_RX_PIN            GPIO_Pin_10

#define UART1_TX_DMA_CHANNEL    DMA1_Channel4   // USART1_TX -> DMA1 Channel4

void uart1_init(uint32_t baudrate);
void uart1_dma_init(void);
void uart1_transmit(uint8_t *tx_buffer, uint16_t tx_length);
void uart1_transmit_non_blocking(uint8_t *tx_buffer, uint16_t tx_length);
uint8_t uart1_is_transmit_complete(void);
void uart1_send_string(char *str);
void uart_receive(uint8_t *rx_buffer, uint16_t rx_length);

#endif
