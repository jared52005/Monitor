/**
******************************************************************************
* @file    rtos_utils.c
* @author  Jaromir Mayer
* @version V1.0
* @date    15-December-2019
* @brief   This file provide basic and often required functions of RTOS
******************************************************************************
* @attention
******************************************************************************
*/ 
  
/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

/* Private defintions -------------------------------------------------------*/

/* Private prototypes -------------------------------------------------------*/

/* Private variables --------------------------------------------------------*/

uint32_t GetTime_ms(void);

/**
* @brief   Wait given amount of miliseconds
* @param   time: How many miliseconds should we wait
*/
void Wait_ms(uint32_t time);

/**
* @brief   Wait given amount of microseconds (100us precision on freefalling 100us timer)
* @param   time: Amount of microseconds within x100us precision
*/
void Wait_us(uint32_t time);

