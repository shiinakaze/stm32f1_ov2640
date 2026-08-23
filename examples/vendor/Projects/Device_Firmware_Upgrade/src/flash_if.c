/**
  ******************************************************************************
  * @file    flash_if.c
  * @author  MCD Application Team
  * @version V4.1.0
  * @date    26-May-2017
  * @brief   specific media access Layer for internal flash
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2017 STMicroelectronics</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */


/* Includes ------------------------------------------------------------------*/
#include "flash_if.h"
#include "dfu_mal.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

#if defined(STM32L1XX_MD) || defined(STM32L1XX_HD)|| defined(STM32L1XX_MD_PLUS)
  uint32_t tHalfPage1[128/4];
  uint32_t tHalfPage2[128/4];
#endif /* STM32L1XX_XD */  

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/*******************************************************************************
* Function Name  : FLASH_If_Init
* Description    : Initializes the Media on the STM32
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
uint16_t FLASH_If_Init(void)
{
  return MAL_OK;
}

/*******************************************************************************
* Function Name  : FLASH_If_Erase
* Description    : Erase sector
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
uint16_t FLASH_If_Erase(uint32_t SectorAddress)
{
#if defined(STM32L1XX_MD) || defined(STM32L1XX_HD)|| defined(STM32L1XX_MD_PLUS)
  FLASH_ClearFlag(FLASH_FLAG_PGAERR | FLASH_FLAG_OPTVERR);
  FLASH_ErasePage(SectorAddress);
#else
  FLASH_ErasePage(SectorAddress);
#endif /* STM32L1XX_XD */
  
  return MAL_OK;
}

/*******************************************************************************
* Function Name  : FLASH_If_Write
* Description    : Write sectors
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
uint16_t FLASH_If_Write(uint32_t SectorAddress, uint32_t DataLength)
{
  uint32_t idx = 0;
#if defined(STM32L1XX_MD) || defined(STM32L1XX_HD)|| defined(STM32L1XX_MD_PLUS)
  __IO uint32_t* malPointer = (uint32_t *)MAL_Buffer;
  __IO uint32_t* memPointer = (uint32_t *)SectorAddress;
  __IO uint32_t memBuffer[32]; /* Temporary buffer holding data that will be written in a half-page space */
  __IO uint32_t* mempBuffer = memBuffer;  
  __IO uint32_t* tmp;
#endif /* STM32L1XX_XD */      
  
  if  (DataLength & 0x3) /* Not an aligned data */
  {
    for (idx = DataLength; idx < ((DataLength & 0xFFFC) + 4); idx++)
    {
      MAL_Buffer[idx] = 0xFF;
    }
  } 
  
#if defined(STM32L1XX_MD) || defined(STM32L1XX_HD)|| defined(STM32L1XX_MD_PLUS) 
  /* Reinitialize the intermediate buffer pointer */
  mempBuffer = memBuffer;
  
  /* If the address is not aligned to half-page fill the first location with existing data */
  if (((uint32_t)memPointer & 0x7F) != 0)
  {
    /* get the aligned address */
    tmp = (uint32_t *)((uint32_t)memPointer & 0xFFFFFF80);
    
    /* Read the first part from the memory */
    while (tmp < memPointer)
    {
      *(uint32_t *)(mempBuffer++) = *(uint32_t *)(tmp++);
    }
  }    
  
  while (malPointer < (uint32_t*)(MAL_Buffer + DataLength))
  {    
    /* Fill with the received buffer */
    while (mempBuffer < (memBuffer + 32))
    {
      /* If there are still data available in the received buffer */
      if (malPointer < ((uint32_t *)MAL_Buffer + DataLength))
      {
        *(uint32_t *)(mempBuffer++) = *(uint32_t *)(malPointer++);
      }
      else /* no more data available in the received buffer: fill remaining with dummy 0 */
      {
        *(uint32_t *)(mempBuffer++) = 0;
      }
    }
   
    /* Write the buffer to the memory*/    
    FLASH_ProgramHalfPage(((uint32_t)memPointer & 0xFFFFFF80), (uint32_t *)(memBuffer));    
    
    /* Increment the memory pointer */ 
    memPointer = (uint32_t *)(((uint32_t)memPointer & 0xFFFFFF80) + (32*4));
    
    /* Reinitialize the intermediate buffer pointer */
    mempBuffer = memBuffer;
  }
  
#else
  
  /* Data received are Word multiple */    
  for (idx = 0; idx <  DataLength; idx = idx + 4)
  {
    FLASH_ProgramWord(SectorAddress, *(uint32_t *)(MAL_Buffer + idx));  
    SectorAddress += 4;
  } 
#endif /* STM32L1XX_XD */
 
  return MAL_OK;
}

/*******************************************************************************
* Function Name  : FLASH_If_Read
* Description    : Read sectors
* Input          : None
* Output         : None
* Return         : buffer address pointer
*******************************************************************************/
uint8_t *FLASH_If_Read (uint32_t SectorAddress, uint32_t DataLength)
{
  return  (uint8_t*)(SectorAddress);
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
