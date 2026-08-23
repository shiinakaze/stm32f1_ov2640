#ifndef _GPIO_H_
#define _GIPO_H_
#include "stm32f4xx.h"
#define KEY_NUMBER         4    //°´¼ü×ÜÊý

#define LED1_RCC           RCC_AHB1Periph_GPIOA
#define LED1_GPIO          GPIOA
#define LED1_GPIO_PIN      GPIO_Pin_4
#define LED1_ONOFF(x)      GPIO_WriteBit(GPIOA,GPIO_Pin_4,x);

#define LED2_RCC           RCC_AHB1Periph_GPIOA
#define LED2_GPIO          GPIOA
#define LED2_GPIO_PIN      GPIO_Pin_6
#define LED2_ONOFF(x)      GPIO_WriteBit(GPIOA,GPIO_Pin_6,x);

#define LED3_RCC           RCC_AHB1Periph_GPIOB
#define LED3_GPIO          GPIOB
#define LED3_GPIO_PIN      GPIO_Pin_6
#define LED3_ONOFF(x)      GPIO_WriteBit(GPIOB,GPIO_Pin_6,x);

#define LED4_RCC           RCC_AHB1Periph_GPIOC
#define LED4_GPIO          GPIOC
#define LED4_GPIO_PIN      GPIO_Pin_6
#define LED4_ONOFF(x)      GPIO_WriteBit(GPIOC,GPIO_Pin_6,x);

typedef struct{
    uint32_t     rcc;
    GPIO_TypeDef *gpio;
    uint16_t     pin;
}Gpio_Info;

void LEDGpio_Init(void);
void LED_All_Switch(BitAction set);
#endif
