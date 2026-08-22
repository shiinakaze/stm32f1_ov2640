/**
  ******************************************************************************
  * @file    stm32f4xx_it.c
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    19-September-2011
  * @brief   Main Interrupt Service Routines.
  *          This file provides all exceptions handler and peripherals interrupt
  *          service routine.
  ******************************************************************************
  * @attention
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2011 STMicroelectronics</center></h2>
  ******************************************************************************
  */ 

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_it.h"
#include "dcmi.h" 
#include "ov2640.h" 
#include "usb_core.h"
#include "usbd_core.h"
#include "stm32f4_discovery.h"
#include "uart.h"
#include "usb_bsp.h"


uint8_t new_frame_trigger = 0;                      //timer set it to 1 every 100ms
u8  Frame_rate;                                    //帧率

extern uint8_t play_status;
extern USB_OTG_CORE_HANDLE                  USB_OTG_dev;
extern uint32_t USBD_OTG_ISR_Handler        (USB_OTG_CORE_HANDLE *pdev);

/******************************************************************************/
/*            Cortex-M4 Processor Exceptions Handlers                         */
/******************************************************************************/

/**
  * @brief   This function handles NMI exception.
  * @param  None
  * @retval None
  */
void NMI_Handler(void)
{
}

/**
  * @brief  This function handles Hard Fault exception.
  * @param  None
  * @retval None
  */
void HardFault_Handler(void)
{
  /* Go to infinite loop when Hard Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Memory Manage exception.
  * @param  None
  * @retval None
  */
void MemManage_Handler(void)
{
  /* Go to infinite loop when Memory Manage exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Bus Fault exception.
  * @param  None
  * @retval None
  */
void BusFault_Handler(void)
{
  /* Go to infinite loop when Bus Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Usage Fault exception.
  * @param  None
  * @retval None
  */
void UsageFault_Handler(void)
{
  /* Go to infinite loop when Usage Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles SVCall exception.
  * @param  None
  * @retval None
  */
void SVC_Handler(void)
{
}

/**
  * @brief  This function handles Debug Monitor exception.
  * @param  None
  * @retval None
  */
void DebugMon_Handler(void)
{
}

/**
  * @brief  This function handles PendSVC exception.
  * @param  None
  * @retval None
  */
void PendSV_Handler(void)
{
}

/**
  * @brief  This function handles SysTick Handler.
  * @param  None
  * @retval None
  */
void SysTick_Handler(void)
{

	//STM_EVAL_LEDToggle(LED5);
	new_frame_trigger = 1;
}

/******************************************************************************/
/*                 STM32Fxxx Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32fxxx.s).                                               */
/******************************************************************************/

/**
  * @brief  This function handles PPP interrupt request.
  * @param  None
  * @retval None
  */
/*void PPP_IRQHandler(void)
{
}*/

/**
  * @brief  This function handles EXTI0_IRQ Handler.
  * @param  None
  * @retval None
  */
void EXTI0_IRQHandler(void)
{
    /* Clear the EXTI line pending bit */
  EXTI_ClearITPendingBit(USER_BUTTON_EXTI_LINE);
  
  //Audio_MAL_Play((uint32_t)(&sinebuf2[0]),48); - working
}

/**
  * @brief  This function handles EXTI15_10_IRQ Handler.
  * @param  None
  * @retval None
  */
void OTG_FS_WKUP_IRQHandler(void)
{
  if(USB_OTG_dev.cfg.low_power)
  {
	/* Reset SLEEPDEEP and SLEEPONEXIT bits */
	SCB->SCR &= (uint32_t)~((uint32_t)(SCB_SCR_SLEEPDEEP_Msk | SCB_SCR_SLEEPONEXIT_Msk));

	/* After wake-up from sleep mode, reconfigure the system clock */
	SystemInit();
    USB_OTG_UngateClock(&USB_OTG_dev);
  }
  EXTI_ClearITPendingBit(EXTI_Line18);
}

/**
  * @brief  This function handles OTG_HS Handler.
  * @param  None
  * @retval None
  */
void OTG_FS_IRQHandler(void)
{
  USBD_OTG_ISR_Handler (&USB_OTG_dev);
}
extern volatile uint8_t new_frame_cap_enabled;
void DCMI_IRQHandler(void)
{
	if(DCMI_GetITStatus(DCMI_IT_FRAME)==SET)  //捕获到一帧图像
	{
//		printf("\r\nCapture an image...\r\n");
		DCMI_Stop();
		
		*Buf_DinPtr=(BUFF_SIZE-DMA_GetCurrDataCounter(DMA2_Stream1))*4; //得到此次数据传输的长度  字节数
		printf("图像大小=%d字节\r\n",(*Buf_DinPtr));                  //串口打印
		
		Buf_DinPtr += BUFF_SIZE;       //后移一个缓存块		
		
		if(Buf_DinPtr==Buf_EndPtr)     //到最后一个了
			Buf_DinPtr=Buf_StartPtr;   //回到开头
		
		if(Buf_DinPtr==Buf_DoutPtr)                //等于输出指针了
		{
			if(Buf_DoutPtr==Buf_StartPtr)          //而且输出指针在开头位置
				Buf_DinPtr=Buf_EndPtr-BUFF_SIZE;   //回退到最后一个
			else			
				Buf_DinPtr -= BUFF_SIZE;           //回退一个
		}
//	printf("\r\nplay_status=[%d]\r\n",play_status);
		if(play_status)
		{
			DCMI_DMA_Init((u32)(Buf_DinPtr+1),BUFF_SIZE,DMA_MemoryDataSize_Word,DMA_MemoryInc_Enable);//DMA配置 		
			DCMI_Start();
		}
		new_frame_cap_enabled=1;
		DCMI_ClearITPendingBit(DCMI_IT_FRAME);                        //清除帧中断
	}
}

/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
