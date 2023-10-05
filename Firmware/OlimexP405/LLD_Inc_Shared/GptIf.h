/*******************************************************************************
  * @file    GptIf.h
  * @author  Jaromir Mayer
  * @version V1.0
  * @date    03-April-2019
  * @brief   This file contains generic definitions for GPT
  ******************************************************************************
  * @attention
  ******************************************************************************  
  */ 

#include <stdint.h>
#include "ErrorCodes.h"

/**
  * @brief  Configures TIM2 as free falling timer with period of 100us
  * @retval None
  */
ErrorCodes Gpt_100us_Init(void);

/**
  * @brief  Get elapsed time in micro seconds since Gpt_100us_Init() was called.
  * @param  microSecs time in micro seconds aligned in 100us
  * @retval None
  */
ErrorCodes Gpt_100us_GetTime(uint64_t* microSecs);
