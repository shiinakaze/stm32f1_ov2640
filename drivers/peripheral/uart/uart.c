#include "uart.h"

static void uart1_gpio_init(void);
static void uart1_usart_init(uint32_t baudrate);

/* 标记当前是否有一次非阻塞 DMA 发送正在进行 */
static volatile uint8_t tx_busy = 0;

void uart1_init(uint32_t baudrate)
{
    /* 时钟使能 */
    RCC_APB2PeriphClockCmd(RCC_APB_UART1_GPIO | RCC_APB_UART1, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHB_UART1_DMA, ENABLE);

    uart1_gpio_init();
    uart1_usart_init(baudrate);
    uart1_dma_init();

    USART_Cmd(USART1, ENABLE);
}

static void uart1_gpio_init(void)
{
    GPIO_InitTypeDef gpio_init_struct;

    /* UART TX */
    gpio_init_struct.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio_init_struct.GPIO_Pin = UART1_TX_PIN;
    gpio_init_struct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(UART1_GPIO, &gpio_init_struct);

    /* UART RX */
    gpio_init_struct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    gpio_init_struct.GPIO_Pin = UART1_RX_PIN;
    GPIO_Init(UART1_GPIO, &gpio_init_struct);
}

static void uart1_usart_init(uint32_t baudrate)
{
    USART_InitTypeDef usart_init_struct;

    usart_init_struct.USART_BaudRate = baudrate;
    usart_init_struct.USART_WordLength = USART_WordLength_8b;
    usart_init_struct.USART_StopBits = USART_StopBits_1;
    usart_init_struct.USART_Parity = USART_Parity_No;
    usart_init_struct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    usart_init_struct.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;

    USART_Init(USART1, &usart_init_struct);

    /* 使能USART DMA发送请求 */
    USART_DMACmd(USART1, USART_DMAReq_Tx, ENABLE);
}

void uart1_dma_init(void)
{
    DMA_InitTypeDef dma_init_struct;

    DMA_DeInit(UART1_TX_DMA_CHANNEL);

    dma_init_struct.DMA_PeripheralBaseAddr = (uint32_t)&USART1->DR;
    dma_init_struct.DMA_MemoryBaseAddr = 0; // 发送时再配置
    dma_init_struct.DMA_DIR = DMA_DIR_PeripheralDST;
    dma_init_struct.DMA_BufferSize = 0; // 发送时再配置
    dma_init_struct.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    dma_init_struct.DMA_MemoryInc = DMA_MemoryInc_Enable;
    dma_init_struct.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    dma_init_struct.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    dma_init_struct.DMA_Mode = DMA_Mode_Normal;
    dma_init_struct.DMA_Priority = DMA_Priority_Medium;
    dma_init_struct.DMA_M2M = DMA_M2M_Disable;

    DMA_Init(UART1_TX_DMA_CHANNEL, &dma_init_struct);

    DMA_Cmd(UART1_TX_DMA_CHANNEL, DISABLE);
}

void uart1_transmit_non_blocking(uint8_t *tx_buffer, uint16_t tx_length)
{
    if (tx_buffer == NULL || tx_length == 0)
        return;

    /* 确保上一次非阻塞发送已经完成，避免覆盖进行中的 DMA */
    while (!uart1_is_transmit_complete())
    {
    }

    /* 关闭DMA，准备重新配置 */
    DMA_Cmd(UART1_TX_DMA_CHANNEL, DISABLE);

    /* 清标志 */
    DMA_ClearFlag(DMA1_FLAG_TC4 | DMA1_FLAG_TE4 | DMA1_FLAG_HT4 | DMA1_FLAG_GL4);

    /* 配置发送地址和长度 */
    UART1_TX_DMA_CHANNEL->CMAR = (uint32_t)tx_buffer;
    UART1_TX_DMA_CHANNEL->CNDTR = tx_length;

    /* 启动DMA */
    DMA_Cmd(UART1_TX_DMA_CHANNEL, ENABLE);
    tx_busy = 1;
}

uint8_t uart1_is_transmit_complete(void)
{
    if (!tx_busy)
        return 1;

    /* DMA 传输完成，且串口移位寄存器把最后一字节发完 */
    if (DMA_GetFlagStatus(DMA1_FLAG_TC4) != RESET &&
        USART_GetFlagStatus(USART1, USART_FLAG_TC) != RESET)
    {
        DMA_Cmd(UART1_TX_DMA_CHANNEL, DISABLE);
        DMA_ClearFlag(DMA1_FLAG_TC4);
        tx_busy = 0;
        return 1;
    }
    return 0;
}

void uart1_transmit(uint8_t *tx_buffer, uint16_t tx_length)
{
    uart1_transmit_non_blocking(tx_buffer, tx_length);
    while (!uart1_is_transmit_complete())
    {
    }
}

void uart1_send_string(char *str)
{
    if (str == NULL)
        return;

    uart1_transmit((uint8_t *)str, strlen(str));
}

void uart_receive(uint8_t *rx_buffer, uint16_t rx_length)
{
    uint16_t i;

    for (i = 0; i < rx_length; i++)
    {
        while (USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == RESET)
        {
        }
        rx_buffer[i] = (uint8_t)USART_ReceiveData(USART1);
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
    uart1_transmit(&c, 1);
    return ch;
}
#endif
