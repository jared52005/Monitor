/*******************************************************************************
* @file    SystemIf.h
* @version V1.0
* @date    28-April-2020
* @brief   Generic defintion of handling global system operations
******************************************************************************
* @attention
******************************************************************************  
*/ 

#include <stdint.h>

/**
* @brief   Jump on system bootloader (STM32 processors, maybe others)
*/
void System_JumpOnSystemBootloader(void);

/**
* @brief   Reset processor
*/
void System_Reset(void);

/**
* @brief   Read out unique ID of MCU or saved ID during manufacture (Should be in OTP)
* @param   *id: Identification of MCU in human readable form
*/
void System_GetUniqueId(char* id);

/**
 * @brief   basic configuration setup
 */
void System_Init(void);

/**
  * @brief This function provides accurate delay (in milliseconds) based 
  *        on variable incremented.
  * @param Delay: specifies the delay time length, in milliseconds.
  * @retval None
  */
void System_Delay_ms(uint32_t Delay);

/**
  * @brief  Povides a tick value in millisecond.
  * @retval tick value
  */
uint32_t System_Get_ms(void);
