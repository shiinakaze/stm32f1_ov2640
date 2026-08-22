/******************************************************************
**	  红龙开发板（V1.0）
**	  Gpio配置文件
**
**	  论    坛：bbs.openmcu.com
**	  旺    宝：www.openmcu.com
**	  邮    箱：support@openmcu.com
**
**    版    本：V1.0
**	  作    者：FXL
**	  完成日期:	2012.7.20
********************************************************************/
#include "stm32f4xx.h"
#include "Gpio.h"

Gpio_Info Gpio_info[KEY_NUMBER]={
    {LED1_RCC,LED1_GPIO,LED1_GPIO_PIN},
    {LED2_RCC,LED2_GPIO,LED2_GPIO_PIN},
    {LED3_RCC,LED3_GPIO,LED3_GPIO_PIN},
    {LED4_RCC,LED4_GPIO,LED4_GPIO_PIN}
};
/********************************************************************************************
*函数名称：void LEDGpio_Init(void)
*
*入口参数：无
*
*出口参数：无
*
*功能说明：LED初始化
*******************************************************************************************/
void LEDGpio_Init(void)
{
    uint8_t i;
    GPIO_InitTypeDef  GPIO_InitStructure;

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;

    for(i=0;i<KEY_NUMBER;i++)
    {
        RCC_AHB1PeriphClockCmd( Gpio_info[i].rcc , ENABLE);
        GPIO_InitStructure.GPIO_Pin = Gpio_info[i].pin;
        GPIO_Init(Gpio_info[i].gpio, &GPIO_InitStructure);
    }
}

void LED_All_Switch(BitAction set)
{
  LED4_ONOFF(set);
  LED3_ONOFF(set);
  LED2_ONOFF(set);
  LED1_ONOFF(set);
}
