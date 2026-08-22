
#include "uart.h"
#include "usbd_usr.h"
#include "usbd_desc.h"
#include "usbd_video_core.h"

#include "stdio.h"
#include "Gpio.h"
#include "ov2640.h" 
#include "timer2_delay.h" 
#include "dcmi.h"  
#include "sram.h"

__ALIGN_BEGIN USB_OTG_CORE_HANDLE  USB_OTG_dev __ALIGN_END;


//JPEG and USB transmission
uint16_t last_jpeg_frame_size = 0;
volatile uint8_t new_frame_cap_enabled = 0;//1 - capture and encoding enabled

void Delay_ms(uint32_t ms);

int main(void)
{	
  USBD_Init(&USB_OTG_dev,      
            USB_OTG_FS_CORE_ID,
            &USR_desc, 
            &VIDEO_cb,
            &USR_cb);
						
  uart_init();
#if USE_CAM
 	SRAM_Init();  
	TIM2_delay_Init(); 
	if(OV2640_Init())       
	{
   printf("OV2640初始化失败\r\n");
		delay_ms(200);
	}else printf("OV2640初始化成功\r\n");
#endif
  while(1)
  {
//    if (new_frame_cap_enabled == 1)//捕获到一帧图像
//    {

//		}
  }
  
  
}

void Delay_ms(uint32_t ms)
{
	volatile uint32_t nCount;
	RCC_ClocksTypeDef RCC_Clocks;
	RCC_GetClocksFreq (&RCC_Clocks);
	nCount=(RCC_Clocks.HCLK_Frequency/10000)*ms;
	for (; nCount!=0; nCount--);
}





