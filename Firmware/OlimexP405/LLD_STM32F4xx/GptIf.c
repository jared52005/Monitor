/*******************************************************************************
  * @file    GptIf.h
  * @author  Jaromir Mayer
  * @version V1.1
  * @date    18-December-2019
  * @brief   This file contains internal timers, which are faster than FreeRTOS is able to handle
  ******************************************************************************
  * @attention
  ******************************************************************************  
  */

#include <stdint.h>
#include <stdio.h>
#include "stm32f4xx.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_tim.h"
#include "GptIf.h"

/*Private variables----------------------------------------------------------*/
static NVIC_InitTypeDef nvicStructure;
static TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
static uint16_t TimerPeriod = 0;
static uint64_t _timerCount = 0;

/**
  * @brief  Configures TIM2 as free falling timer to achieve higher time precision in FreeRTOS
  * @param  None
  * @retval None
  */
ErrorCodes Gpt_100us_Init()
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	
  /* Compute the value to be set in ARR register to generate signal frequency at "freq" Khz */
	//TIM2 connected on APB1 = 42MHz
	//100uS = 10000Hz * 2 (because we are on APB1 instead APB2)
  TimerPeriod = (SystemCoreClock / 20000) - 1;
	_timerCount = 0;
	
  /* Time Base configuration */
  TIM_TimeBaseStructure.TIM_Prescaler = 0x0;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Down;
  TIM_TimeBaseStructure.TIM_Period = TimerPeriod;
  TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
  TIM_TimeBaseStructure.TIM_RepetitionCounter = 0; //Valid only for TIM1 and TIM8
  TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
	
	//Setup interrupt requests for TIM2
  nvicStructure.NVIC_IRQChannel = TIM2_IRQn;
  nvicStructure.NVIC_IRQChannelPreemptionPriority = 0;
  nvicStructure.NVIC_IRQChannelSubPriority = 1;
  nvicStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&nvicStructure);
	TIM_ITConfig(TIM2, TIM_IT_Update , ENABLE);
	
	TIM_Cmd(TIM2, ENABLE);
	return ERROR_OK;
}

/**
  * @brief  Get elapsed time in micro seconds since Gpt_100us_Init() was called.
  * @param  microSecs time in micro seconds aligned in 100us
  * @retval None
  */
ErrorCodes Gpt_100us_GetTime(uint64_t* microSecs)
{
	//Timer is set on 100uS, so we see how many times has timer went through
	//To get time in microsecodns, we need to multiply it by 100;
	*microSecs = (_timerCount * 100);
	return ERROR_OK;
}

void TIM2_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)
  {
      TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
      _timerCount++;
  }
}
