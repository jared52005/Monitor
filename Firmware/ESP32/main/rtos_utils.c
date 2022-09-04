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
#include "rtos_utils.h"
//*****************************************************************************
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
//*****************************************************************************

/* Private defintions -------------------------------------------------------*/

/* Private prototypes -------------------------------------------------------*/

/* Private variables --------------------------------------------------------*/

/**
* @brief   Get amount of miliseconds since start of device
*/
uint32_t GetTime_ms()
{
	return (uint32_t)((xTaskGetTickCount() * portTICK_PERIOD_MS));
}
