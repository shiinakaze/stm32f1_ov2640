#include "uart.h"

static void UART1_GPIO_Init(void);
static void UART1_USART_Init(uint32_t baudrate);

void UART1_Init(uint32_t baudrate)
{
    /* 时钟使能 */
    RCC_APB2PeriphClockCmd(RCC_APB_UART1_GPIO | RCC_APB_UART1, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHB_UART1_DMA, ENABLE);

    UART1_GPIO_Init();
    UART1_USART_Init(baudrate);
    UART1_DMA_Init();

    USART_Cmd(USART1, ENABLE);
}

static void UART1_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /* UART TX */
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Pin = UART1_TX_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(UART1_GPIO, &GPIO_InitStructure);

    /* UART RX */
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_InitStructure.GPIO_Pin = UART1_RX_PIN;
    GPIO_Init(UART1_GPIO, &GPIO_InitStructure);
}

static void UART1_USART_Init(uint32_t baudrate)
{
    USART_InitTypeDef USART_InitStructure;

    USART_InitStructure.USART_BaudRate = baudrate;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;

    USART_Init(USART1, &USART_InitStructure);

    /* 使能USART DMA发送请求 */
    USART_DMACmd(USART1, USART_DMAReq_Tx, ENABLE);
}

void UART1_DMA_Init(void)
{
    DMA_InitTypeDef DMA_InitStructure;

    DMA_DeInit(UART1_TX_DMA_CHANNEL);

    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&USART1->DR;
    DMA_InitStructure.DMA_MemoryBaseAddr = 0; // 发送时再配置
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
    DMA_InitStructure.DMA_BufferSize = 0; // 发送时再配置
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
    DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;

    DMA_Init(UART1_TX_DMA_CHANNEL, &DMA_InitStructure);

    DMA_Cmd(UART1_TX_DMA_CHANNEL, DISABLE);
}

void UART1_Transmit(uint8_t *TxBuffer, uint16_t TxLength)
{
    if (TxBuffer == NULL || TxLength == 0)
        return;

    /* 关闭DMA，准备重新配置 */
    DMA_Cmd(UART1_TX_DMA_CHANNEL, DISABLE);

    /* 清标志 */
    DMA_ClearFlag(DMA1_FLAG_TC4 | DMA1_FLAG_TE4 | DMA1_FLAG_HT4 | DMA1_FLAG_GL4);

    /* 配置发送地址和长度 */
    UART1_TX_DMA_CHANNEL->CMAR = (uint32_t)TxBuffer;
    UART1_TX_DMA_CHANNEL->CNDTR = TxLength;

    /* 启动DMA */
    DMA_Cmd(UART1_TX_DMA_CHANNEL, ENABLE);

    /* 等待发送完成 */
    while (DMA_GetFlagStatus(DMA1_FLAG_TC4) == RESET)
    {
    }

    /* 清完成标志 */
    DMA_ClearFlag(DMA1_FLAG_TC4);

    /* 等待串口真正发完最后一个字节 */
    while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET)
    {
    }

    DMA_Cmd(UART1_TX_DMA_CHANNEL, DISABLE);
}

void UART1_SendString(char *str)
{
    if (str == NULL)
        return;

    UART1_Transmit((uint8_t *)str, strlen(str));
}

void UART_Receive(uint8_t *RxBuffer, uint16_t RxLength)
{
    uint16_t i;

    for (i = 0; i < RxLength; i++)
    {
        while (USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == RESET)
        {
        }
        RxBuffer[i] = (uint8_t)USART_ReceiveData(USART1);
    }
}

#if 1
#pragma import(__use_no_semihosting)

/* 标准库需要的支持 */
struct __FILE
{
    int handle;
};

FILE __stdout;

/* 避免半主机模式 */
void _sys_exit(int x)
{
    x = x;
}

/* 重定向 printf 到 USART1 */
int fputc(int ch, FILE *f)
{
    uint8_t c = (uint8_t)ch;
    UART1_Transmit(&c, 1);
    return ch;
}
#endif
