/**
  ******************************************************************************
  * @file    nor_if.c
  * @author  MCD Application Team
  * @version V4.1.0
  * @date    26-May-2017
  * @brief   specific media access Layer for NOR flash
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
  

#include "platform_config.h"

#ifdef USE_STM3210E_EVAL

/* Includes ------------------------------------------------------------------*/
#include "fsmc_nor.h"
#include "nor_if.h"
#include "dfu_mal.h"
#include "stm32f10x_fsmc.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
extern NOR_IDTypeDef NOR_ID;

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/*******************************************************************************
* Function Name  : NOR_If_Init
* Description    : Initializes the Media on the STM32
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
uint16_t NOR_If_Init(void)
{
  /* Configure FSMC Bank1 NOR/SRAM2 */
  FSMC_NOR_Init();

  /* Enable FSMC Bank1 NOR/SRAM2 */
  FSMC_NORSRAMCmd(FSMC_Bank1_NORSRAM2, ENABLE);

  return MAL_OK;
}

/*******************************************************************************
* Function Name  : NOR_If_Erase
* Description    : Erase sector
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
uint16_t NOR_If_Erase(uint32_t Address)
{
  /* Erase the destination memory */
  FSMC_NOR_EraseBlock(Address & 0x00FFFFFF);    

  return MAL_OK;
}

/*******************************************************************************
* Function Name  : NOR_If_Write
* Description    : Write sectors
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
uint16_t NOR_If_Write(uint32_t Address, uint32_t DataLength)
{
  if ((DataLength & 1) == 1) /* Not an aligned data */
  {
    DataLength += 1;
    MAL_Buffer[DataLength-1] = 0xFF;
  }
  
  FSMC_NOR_WriteBuffer((uint16_t *)MAL_Buffer, (Address&0x00FFFFFF), DataLength >> 1);  
  
  return MAL_OK;
}

/*******************************************************************************
* Function Name  : NOR_If_Read
* Description    : Read sectors
* Input          : None
* Output         : None
* Return         : buffer address pointer
*******************************************************************************/
uint8_t *NOR_If_Read(uint32_t Address, uint32_t DataLength)
{
  return  (uint8_t*)(Address);
}

#endif

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
