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
//#include "GptIf.h"
//*****************************************************************************
#include "FreeRTOS.h"
#include "task.h"
//*****************************************************************************

/* Private defintions -------------------------------------------------------*/

/* Private prototypes -------------------------------------------------------*/

/* Private variables --------------------------------------------------------*/

/**
* @brief   Get amount of miliseconds since start of device
*/
uint32_t GetTime_ms()
{
	return (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
}

/**
* @brief   Wait given amount of miliseconds
* @param   time: How many miliseconds should we wait
*/
void Wait_ms(uint32_t time)
{
	uint32_t currentTime;
	uint32_t targetTime;
	if (time == 0)
	{
		//Nothing to wait on
		return;
	}
	targetTime = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS) + time;
	for (;;)
	{
		currentTime = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
		if (targetTime > currentTime)
		{
			taskYIELD();
		}
		else
		{
			return;
		}
	}
}

