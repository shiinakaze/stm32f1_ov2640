
#include "timer2_delay.h"

/*-------------------------------------------------*/
/*函数名：定时器2延时函数初始化                    */
/*参  数：无                                       */
/*返回值：无                                       */
/*-------------------------------------------------*/
void TIM2_delay_Init(void)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2,ENABLE);           //使能TIM2时钟	
  TIM_TimeBaseInitStructure.TIM_Period = 0xFFFFFFFF-1; 	        //自动重装载值
	TIM_TimeBaseInitStructure.TIM_Prescaler=6-1;                  //定时器6分频=12M
	TIM_TimeBaseInitStructure.TIM_CounterMode=TIM_CounterMode_Up; //向上计数模式
	TIM_TimeBaseInitStructure.TIM_ClockDivision=TIM_CKD_DIV1;     //1分频
	TIM_TimeBaseInit(TIM2,&TIM_TimeBaseInitStructure);            //按配置设置TIM2	
}

/*-------------------------------------------------*/
/*函数名：延迟微秒函数                             */
/*参  数：us：延时多少微秒                         */
/*返回值：无                                       */
/*-------------------------------------------------*/
void delay_us(u32 us)
{			
	TIM_SetAutoreload(TIM2,us*12);                     //12M频率  12个数使1us
	TIM_SetCounter(TIM2,0);                            //清空计数器
	TIM_Cmd(TIM2,ENABLE);                              //开始计数
	while(TIM_GetFlagStatus(TIM2,TIM_IT_Update)!=SET); //等待溢出
	TIM_ClearFlag(TIM2,TIM_IT_Update);                 //清除标志位
	TIM_SetCounter(TIM2,0);                            //清空计数器
	TIM_Cmd(TIM2,DISABLE);                             //关闭计数器
}

/*-------------------------------------------------*/
/*函数名：延迟毫秒函数                             */
/*参  数：us：延时多少微秒                         */
/*返回值：无                                       */
/*-------------------------------------------------*/
void delay_ms(u32 ms)
{			
	TIM_SetAutoreload(TIM2,ms*12*1000);                //12M频率  12个数使1us  再乘以1000是1ms
	TIM_SetCounter(TIM2,0);                            //清空计数器
	TIM_Cmd(TIM2,ENABLE);                              //开始计数
	while(TIM_GetFlagStatus(TIM2,TIM_IT_Update)!=SET); //等待溢出
	TIM_ClearFlag(TIM2,TIM_IT_Update);                 //清除标志位
	TIM_SetCounter(TIM2,0);                            //清空计数器
	TIM_Cmd(TIM2,DISABLE);                             //关闭计数器
}

