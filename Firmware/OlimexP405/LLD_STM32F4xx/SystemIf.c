/**
******************************************************************************
* @file    SystemIf.c
* @version V1.0
* @date    29-April-2020
* @brief   This file provides stubbed firmware for Windows Emulator
******************************************************************************
* @attention
******************************************************************************
*/ 
  
/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <string.h>
#include "SystemIf.h"
#include "GpioIf.h"
#include "stm32f4xx.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_gpio.h"
#include "ErrorCodes.h"

/* Private defintions -------------------------------------------------------*/
#define SYSMEM_ADDRESS  (uint32_t)0x1FFF0000    /* Address of System Memory (ST Bootloader) */
#define APP_ADDRESS     (uint32_t)0x08010200    /* Start address of application space in flash */
#define STM32_UUID      ((uint8_t *)0x1FFF7A10) /* Universal Unique ID for System */

/* Private prototypes -------------------------------------------------------*/
typedef void (*pFunction)(void);

/* Private prototypes -------------------------------------------------------*/

void System_Reset(void)
{
	NVIC_SystemReset();
	return;
}

/**
* @brief   Jump on system bootloader (STM32 processors, maybe others)
*/
void System_JumpOnSystemBootloader(void)
{
	uint32_t  JumpAddress = *(__IO uint32_t*)(SYSMEM_ADDRESS + 4);
  pFunction Jump = (pFunction)JumpAddress;
    
  RCC_DeInit();
    
  SysTick->CTRL = 0;
  SysTick->LOAD = 0;
  SysTick->VAL  = 0;
       
  __set_MSP(*(__IO uint32_t*)SYSMEM_ADDRESS);
  Jump();
  
	while(1);
}
