#ifndef __UART_H
#define __UART_H

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

void UART1_Init(uint32_t baudrate);
void UART1_DMA_Init(void);
void UART1_Transmit(uint8_t *TxBuffer, uint16_t TxLength);
void UART1_SendString(char *str);
void UART_Receive(uint8_t *RxBuffer, uint16_t RxLength);

#endif
