

#include "ov2640.h"
#include "ov2640cfg.h"  
#include "timer2_delay.h"	 
#include "sccb_bus.h"	
#include "sram.h" 
#include "dcmi.h" 


u32 Ov2640_buff[BUFF_NUM][BUFF_SIZE]     __attribute__((at(Bank1_SRAM3_ADDR)));  

u32 *Buf_StartPtr;   //指向Ov2640_buff开始位置的指针
u32 *Buf_EndPtr;     //指向Ov2640_buff结束位置的指针
u32 *Buf_DinPtr;     //写入数据位置的指针
u32 *Buf_DoutPtr;    //读出数据位置的指针
 
const u16 jpeg_img_size_tbl[][2]=     //分辨率列表
{
	176,144,	//QCIF     
	160,120,	//QQVGA    
	352,288,	//CIF      
	320,240,	//QVGA     
	640,480,	//VGA      
	800,600,	//SVGA     
	1024,768,	//XGA      
	1280,1024,	//SXGA     
	1600,1200,	//UXGA     
}; 

static void _delay(uint32_t dl)
{
	unsigned int i,y;
	for(i = 0; i < 1000; i++)
	{
		for(y = 0; y < dl; y++);
	}
}


/**
  * @brief  写OV2640寄存器
  * @param  reg:要写入的寄存器;data:写入的数据
  * @retval 0:success other:error
  */
uint8_t ov2640_write_reg(uint8_t reg, uint8_t data)
{
//start:
	systick_delay_us(10);
	sccb_bus_start();                          //发送SCCB 总线开始传输命令
	if(sccb_bus_write_byte(0x60) == 0)         //写地址
	{
		sccb_bus_stop();                         //发送SCCB 总线停止传输命令
	//	goto start;
		return 1;//错误返回
	}
	systick_delay_us(10);
	if(sccb_bus_write_byte(reg) == 0)//ID
	{
		sccb_bus_stop();                                 //发送SCCB 总线停止传输命令
	//	goto start;
		return 2;                                        //错误返回
	}
	systick_delay_us(10);
	if(sccb_bus_write_byte(data)==0)                    //写数据到积存器
	{
		sccb_bus_stop();                                  //发送SCCB 总线停止传输命令
		//goto start;
		return 3;//错误返回
	}
	sccb_bus_stop();                                    //发送SCCB 总线停止传输命令	
	return 0;//成功返回
}

/**
  * @brief  读OV2640寄存器
  * @param  reg:要读取的寄存器;ret_reg:指向读取的数据
  * @retval 0:success other:error
            
  */
uint8_t ov2640_read_reg(uint8_t reg, uint8_t *ret_reg)
{
//start:
	systick_delay_us(10);
	
	sccb_bus_start();
	if(sccb_bus_write_byte(0x60) == 0)                 //写地址
	{
		sccb_bus_stop();                                  //发送SCCB 总线停止传输命令
		//goto start;
		return 1;                                         //错误返回
	}
	systick_delay_us(10);
	if(sccb_bus_write_byte(reg)==0)//ID
	{
		sccb_bus_stop();                                   //发送SCCB 总线停止传输命令
		//goto start;
		return 2;                                          //错误返回
	}
	sccb_bus_stop();                                     //发送SCCB 总线停止传输命令	
	systick_delay_us(10);	
	//设置寄存器地址后，才是读
	sccb_bus_start();
	if(sccb_bus_write_byte(0x61)==0)                      //读地址
	{
		sccb_bus_stop();                                    //发送SCCB 总线停止传输命令
		//goto start;
		return 3;                                           //错误返回
	}
	systick_delay_us(10);
	*ret_reg = sccb_bus_read_byte();                      //返回读到的值
	sccb_bus_send_noack();                                //发送NACK命令
	sccb_bus_stop();                                      //发送SCCB 总线停止传输命令
	return 0;                                             //成功返回
}

/**
  * @brief  software reset
  * @param  None
  * @retval None
  */
void ov2640_reset(void)
{
	ov2640_write_reg(0xFF, 0x01);
	ov2640_write_reg(0x12, 0x80);
	_delay(1000);
}
/**
  * @brief  读取ID
  * @param  None
  * @retval temp
           -Camera器件ID
  */
uint16_t ov2640_get_id(void)
{
	uint8_t pidh=0,pidl=0;
	uint16_t temp=0;
	ov2640_write_reg(0xff, 0x01);
	ov2640_read_reg(0x0a,&pidh);
	ov2640_read_reg(0x0b,&pidl);
	temp=(uint16_t)(((uint16_t)(pidh<<8)) | pidl);

	return temp;
}
 
void OV2640_JPEG_Mode(void) 
{
	u16 i=0;
	//设置:YUV422格式
	for(i=0;i<(sizeof(ov2640_yuv422_reg_tbl)/2);i++)
	{
		ov2640_write_reg(ov2640_yuv422_reg_tbl[i][0],ov2640_yuv422_reg_tbl[i][1]); 
	} 
	
	//设置:输出JPEG数据
	for(i=0;i<(sizeof(ov2640_jpeg_reg_tbl)/2);i++)
	{
		ov2640_write_reg(ov2640_jpeg_reg_tbl[i][0],ov2640_jpeg_reg_tbl[i][1]); 
    systick_delay_us(20);		
	}  
}


void OV2640_RGB565_Mode(void) 
{
	u16 i=0;
	for(i=0;i<(sizeof(ov2640_rgb565_reg_tbl)/2);i++)
	{
		ov2640_write_reg(ov2640_rgb565_reg_tbl[i][0],ov2640_rgb565_reg_tbl[i][1]); 
	} 
} 
 
const static u8 OV2640_AUTOEXPOSURE_LEVEL[5][8]=
{
	{
		0xFF,0x01,
		0x24,0x20,
		0x25,0x18,
		0x26,0x60,
	},
	{
		0xFF,0x01,
		0x24,0x34,
		0x25,0x1c,
		0x26,0x00,
	},
	{
		0xFF,0x01,	
		0x24,0x3e,	
		0x25,0x38,
		0x26,0x81,
	},
	{
		0xFF,0x01,
		0x24,0x48,
		0x25,0x40,
		0x26,0x81,
	},
	{
		0xFF,0x01,	
		0x24,0x58,	
		0x25,0x50,	
		0x26,0x92,	
	},
}; 


void OV2640_Auto_Exposure(u8 level)
{  
	u8 i;
	u8 *p=(u8*)OV2640_AUTOEXPOSURE_LEVEL[level];
	for(i=0;i<4;i++)
	{ 
		ov2640_write_reg(p[i*2],p[i*2+1]); 
	} 
}  


void OV2640_Light_Mode(u8 mode)
{
	u8 regccval=0x5E;//Sunny 
	u8 regcdval=0x41;
	u8 regceval=0x54;
	switch(mode)
	{ 
		case 0://auto 
			ov2640_write_reg(0xFF,0x00);	 
			ov2640_write_reg(0xC7,0x00);//AWB ON 
			return;  	
		case 2://cloudy
			regccval=0x65;
			regcdval=0x41;
			regceval=0x4F;
			break;	
		case 3://office
			regccval=0x52;
			regcdval=0x41;
			regceval=0x66;
			break;	
		case 4://home
			regccval=0x42;
			regcdval=0x3F;
			regceval=0x71;
			break;	
	}
	ov2640_write_reg(0xFF,0x00);	 
	ov2640_write_reg(0xC7,0x40);	//AWB OFF 
	ov2640_write_reg(0xCC,regccval); 
	ov2640_write_reg(0xCD,regcdval); 
	ov2640_write_reg(0xCE,regceval);  
}

void OV2640_Color_Saturation(u8 sat)
{ 
	u8 reg7dval=((sat+2)<<4)|0x08;
	ov2640_write_reg(0xFF,0x00);		
	ov2640_write_reg(0x7C,0x00);		
	ov2640_write_reg(0x7D,0x02);				
	ov2640_write_reg(0x7C,0x03);			
	ov2640_write_reg(0x7D,reg7dval);			
	ov2640_write_reg(0x7D,reg7dval); 		
}

/*-------------------------------------------------*/
/*函数名：亮度设置                                 */
/*参  数：bright：亮度                             */
/*        0:-2                                     */
/*        1:-1                                     */
/*        2:0                                      */
/*        3:+1                                     */
/*        4:+2                                     */
/*返回值：无                                       */
/*-------------------------------------------------*/
void OV2640_Brightness(u8 bright)
{
  ov2640_write_reg(0xff, 0x00);
  ov2640_write_reg(0x7c, 0x00);
  ov2640_write_reg(0x7d, 0x04);
  ov2640_write_reg(0x7c, 0x09);
  ov2640_write_reg(0x7d, bright<<4); 
  ov2640_write_reg(0x7d, 0x00); 
}

/*-------------------------------------------------*/
/*函数名：对比度设置                               */
/*参  数：contrast：对比度                         */
/*        0:-2                                     */
/*        1:-1                                     */
/*        2:0                                      */
/*        3:+1                                     */
/*        4:+2                                     */
/*返回值：无                                       */
/*-------------------------------------------------*/
void OV2640_Contrast(u8 contrast)
{
	u8 reg7d0val=0x20;//默认为普通模式
	u8 reg7d1val=0x20;
  	switch(contrast)
	{
		case 0://-2
			reg7d0val=0x18;	 	 
			reg7d1val=0x34;	 	 
			break;	
		case 1://-1
			reg7d0val=0x1C;	 	 
			reg7d1val=0x2A;	 	 
			break;	
		case 3://1
			reg7d0val=0x24;	 	 
			reg7d1val=0x16;	 	 
			break;	
		case 4://2
			reg7d0val=0x28;	 	 
			reg7d1val=0x0C;	 	 
			break;	
	}
	ov2640_write_reg(0xff,0x00);
	ov2640_write_reg(0x7c,0x00);
	ov2640_write_reg(0x7d,0x04);
	ov2640_write_reg(0x7c,0x07);
	ov2640_write_reg(0x7d,0x20);
	ov2640_write_reg(0x7d,reg7d0val);
	ov2640_write_reg(0x7d,reg7d1val);
	ov2640_write_reg(0x7d,0x06);
}

  
void OV2640_Special_Effects(u8 eft)
{
	u8 reg7d0val=0x00;//默认为普通模式
	u8 reg7d1val=0x80;
	u8 reg7d2val=0x80; 
	switch(eft)
	{
		case 1://负片
			reg7d0val=0x40; 
			break;	
		case 2://黑白
			reg7d0val=0x18; 
			break;	 
		case 3://偏红色
			reg7d0val=0x18; 
			reg7d1val=0x40;
			reg7d2val=0xC0; 
			break;	
		case 4://偏绿色
			reg7d0val=0x18; 
			reg7d1val=0x40;
			reg7d2val=0x40; 
			break;	
		case 5://偏蓝色
			reg7d0val=0x18; 
			reg7d1val=0xA0;
			reg7d2val=0x40; 
			break;	
		case 6://复古
			reg7d0val=0x18; 
			reg7d1val=0x40;
			reg7d2val=0xA6; 
			break;	 
	}
	ov2640_write_reg(0xff,0x00);
	ov2640_write_reg(0x7c,0x00);
	ov2640_write_reg(0x7d,reg7d0val);
	ov2640_write_reg(0x7c,0x05);
	ov2640_write_reg(0x7d,reg7d1val);
	ov2640_write_reg(0x7d,reg7d2val); 
}


void OV2640_Color_Bar(u8 sw)
{
	u8 reg;
	ov2640_write_reg(0xFF,0x01);
	ov2640_read_reg(0x12,&reg);
	reg&=~(1<<1);
	if(sw)reg|=1<<1; 
	ov2640_write_reg(0x12,reg);
}

void OV2640_Window_Set(u16 sx,u16 sy,u16 width,u16 height)
{
	u16 endx;
	u16 endy;
	u8 temp; 
	endx=sx+width/2;	//V*2
 	endy=sy+height/2;
	
	ov2640_write_reg(0xFF,0x01);			
	ov2640_read_reg(0x03,&temp);
	temp&=0xF0;
	temp|=((endy&0x03)<<2)|(sy&0x03);
	ov2640_write_reg(0x03,temp);				//设置Vref的start和end的最低2位
	ov2640_write_reg(0x19,sy>>2);			//设置Vref的start高8位
	ov2640_write_reg(0x1A,endy>>2);			//设置Vref的end的高8位
	
	ov2640_read_reg(0x32,&temp);
	temp&=0xC0;
	temp|=((endx&0x07)<<3)|(sx&0x07);
	ov2640_write_reg(0x32,temp);				//设置Href的start和end的最低3位
	ov2640_write_reg(0x17,sx>>3);			//设置Href的start高8位
	ov2640_write_reg(0x18,endx>>3);			//设置Href的end的高8位
}


u8 OV2640_OutSize_Set(u16 width,u16 height)
{
	u16 outh;
	u16 outw;
	u8 temp; 
	if(width%4)return 1;
	if(height%4)return 2;
	outw=width/4;
	outh=height/4; 
	ov2640_write_reg(0xFF,0x00);	
	ov2640_write_reg(0xE0,0x04);			
	ov2640_write_reg(0x5A,outw&0xFF);		//设置OUTW的低八位
	ov2640_write_reg(0x5B,outh&0xFF);		//设置OUTH的低八位
	temp=(outw>>8)&0x03;
	temp|=(outh>>6)&0x04;
	ov2640_write_reg(0x5C,temp);				//设置OUTH/OUTW的高位 
	ov2640_write_reg(0xE0,0x00);	
	return 0;
}


u8 OV2640_ImageWin_Set(u16 offx,u16 offy,u16 width,u16 height)
{
	u16 hsize;
	u16 vsize;
	u8 temp; 
	if(width%4)return 1;
	if(height%4)return 2;
	hsize=width/4;
	vsize=height/4;
	ov2640_write_reg(0xFF,0x00);	
	ov2640_write_reg(0xE0,0x04);					
	ov2640_write_reg(0x51,hsize&0xFF);		//设置H_SIZE的低八位
	ov2640_write_reg(0x52,vsize&0xFF);		//设置V_SIZE的低八位
	ov2640_write_reg(0x53,offx&0xFF);		//设置offx的低八位
	ov2640_write_reg(0x54,offy&0xFF);		//设置offy的低八位
	temp=(vsize>>1)&0x80;
	temp|=(offy>>4)&0x70;
	temp|=(hsize>>5)&0x08;
	temp|=(offx>>8)&0x07; 
	ov2640_write_reg(0x55,temp);				//设置H_SIZE/V_SIZE/OFFX,OFFY的高位
	ov2640_write_reg(0x57,(hsize>>2)&0x80);	//设置H_SIZE/V_SIZE/OFFX,OFFY的高位
	ov2640_write_reg(0xE0,0x00);	
	return 0;
}

u8 OV2640_ImageSize_Set(u16 width,u16 height)
{ 
	u8 temp; 
	ov2640_write_reg(0xFF,0x00);			
	ov2640_write_reg(0xE0,0x04);			
	ov2640_write_reg(0xC0,(width)>>3&0xFF);		//设置HSIZE的10:3位
	ov2640_write_reg(0xC1,(height)>>3&0xFF);		//设置VSIZE的10:3位
	temp=(width&0x07)<<3;
	temp|=height&0x07;
	temp|=(width>>4)&0x80; 
	ov2640_write_reg(0x8C,temp);	
	ov2640_write_reg(0xE0,0x00);				 
	return 0;
}
	
void OV2640_Buff_Init(void)
{
	Buf_StartPtr=Ov2640_buff[0];       
	Buf_EndPtr=Ov2640_buff[BUFF_NUM-1];  
	Buf_DinPtr=Buf_StartPtr;           
	Buf_DoutPtr=Buf_DinPtr;            
}


u8 OV2640_Init(void)
{ 
	u16 i=0;
	uint16_t ovid;
	sccb_bus_init();        		            //初始化SCCB 的IO口	 
	delay_ms(50); 
  ov2640_reset();
	systick_delay_ms(5);  
  ovid = ov2640_get_id();
//	printf("\r\nCamera ID:%0x%x\r\n",ovid);
	 //while(1){;}
	for(i=0;i<sizeof(ov2640_svga_init_reg_tbl)/2;i++)
	{
		ov2640_write_reg(ov2640_svga_init_reg_tbl[i][0],ov2640_svga_init_reg_tbl[i][1]);
	  systick_delay_us(40);		
	} 	
	
	OV2640_Buff_Init();		//缓冲区初始化
	OV2640_JPEG_Mode();		//JPEG模式
	OV2640_DCMI_Init();		//DCMI配置
	DCMI_DMA_Init((u32)(Buf_DinPtr+1),BUFF_SIZE,DMA_MemoryDataSize_Word,DMA_MemoryInc_Enable);//DMA配置   
	OV2640_OutSize_Set(jpeg_img_size_tbl[3][0],jpeg_img_size_tbl[3][1]);                       //分辨率列表数组脚标3：320*240
//	OV2640_Auto_Exposure(1);
//	OV2640_Brightness(3);
//	OV2640_Light_Mode(3);
	return 0; 
} 
