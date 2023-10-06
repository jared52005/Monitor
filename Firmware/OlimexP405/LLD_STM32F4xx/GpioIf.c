/**
******************************************************************************
* @file    GpioIf.c
* @version V1.0
* @date    29-April-2020
* @brief   Implemenation of GPIO control for STM32F4xx boards
******************************************************************************
* @attention
******************************************************************************
*/ 
  
/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <string.h>
#include "GpioIf.h"
#include "stm32f4xx.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_gpio.h"

/* Private defintions -------------------------------------------------------*/
#define LEDn                             9
#ifdef STM32F40GEVAL
#define LED_RED_PIN                         GPIO_Pin_9
#define LED_RED_GPIO_PORT                   GPIOI
#define LED_RED_GPIO_CLK                    RCC_AHB1Periph_GPIOI
  
#define LED_GREEN_PIN                       GPIO_Pin_6
#define LED_GREEN_GPIO_PORT                 GPIOG
#define LED_GREEN_GPIO_CLK                  RCC_AHB1Periph_GPIOG
  
#define LED_BLUE_PIN                        GPIO_Pin_7
#define LED_BLUE_GPIO_PORT                  GPIOC
#define LED_BLUE_GPIO_CLK                   RCC_AHB1Periph_GPIOC
  
#define LED_ORANGE_PIN                      GPIO_Pin_8
#define LED_ORANGE_GPIO_PORT                GPIOG
#define LED_ORANGE_GPIO_CLK                 RCC_AHB1Periph_GPIOG
#endif

#if defined(OLIMEXP405_Cv10)
#define LED_RED_PIN                         GPIO_Pin_12
#define LED_RED_GPIO_PORT                   GPIOC
#define LED_RED_GPIO_CLK                    RCC_AHB1Periph_GPIOC
  
#define LED_GREEN_PIN                       GPIO_Pin_12
#define LED_GREEN_GPIO_PORT                 GPIOC
#define LED_GREEN_GPIO_CLK                  RCC_AHB1Periph_GPIOC
  
#define LED_BLUE_PIN                        NULL
#define LED_BLUE_GPIO_PORT                  NULL
#define LED_BLUE_GPIO_CLK                   NULL
  
#define LED_ORANGE_PIN                      NULL
#define LED_ORANGE_GPIO_PORT                NULL
#define LED_ORANGE_GPIO_CLK                 NULL

#define CAN_LBK_PIN                         NULL
#define CAN_LBK_GPIO_PORT                   NULL
#define CAN_LBK_GPIO_CLK                    NULL
  
#define CAN_RS_PIN                          NULL
#define CAN_RS_GPIO_PORT                    NULL
#define CAN_RS_GPIO_CLK                     NULL

#define KLINE_K_PIN                         GPIO_Pin_10
#define KLINE_K_GPIO_PORT                   GPIOB
#define KLINE_K_GPIO_CLK                    RCC_AHB1Periph_GPIOB
  
#define KLINE_L_PIN                         NULL
#define KLINE_L_GPIO_PORT                   NULL
#define KLINE_L_GPIO_CLK                    NULL

#define GPIO_VBAT_PIN                       GPIO_Pin_5
#define GPIO_VBAT_GPIO_PORT                 GPIOB
#define GPIO_VBAT_GPIO_CLK                  RCC_AHB1Periph_GPIOB

#define GPIO_IGN_PIN                        GPIO_Pin_2
#define GPIO_IGN_GPIO_PORT                  GPIOB
#define GPIO_IGN_GPIO_CLK                   RCC_AHB1Periph_GPIOB
#endif

/* Private prototypes -------------------------------------------------------*/
typedef void (*pFunction)(void);

/* Private variables --------------------------------------------------------*/

/** @defgroup STM32F4_DISCOVERY_LOW_LEVEL_Private_Variables
  * @{
  */ 
GPIO_TypeDef* GPIO_PORT[LEDn] = {
	LED_RED_GPIO_PORT,
	LED_GREEN_GPIO_PORT,
	CAN_LBK_GPIO_PORT,
	CAN_RS_GPIO_PORT,
	KLINE_K_GPIO_PORT,
	KLINE_L_GPIO_PORT,
	LED_BLUE_GPIO_PORT,
	GPIO_VBAT_GPIO_PORT,
	GPIO_IGN_GPIO_PORT,
};
const uint16_t GPIO_PIN[LEDn] = {
	LED_RED_PIN,
	LED_GREEN_PIN, 
	CAN_LBK_PIN,
	CAN_RS_PIN,
	KLINE_K_PIN,
	KLINE_L_PIN,
	LED_BLUE_PIN, 
	GPIO_VBAT_PIN,
	GPIO_IGN_PIN,
};
const uint32_t GPIO_CLK[LEDn] = {
	LED_RED_GPIO_CLK,
	LED_GREEN_GPIO_CLK,
	CAN_LBK_GPIO_CLK,
	CAN_RS_GPIO_CLK,
	KLINE_K_GPIO_CLK,
	KLINE_L_GPIO_CLK,
	LED_BLUE_GPIO_CLK,
	GPIO_VBAT_GPIO_CLK,
	GPIO_IGN_GPIO_CLK,
};

void GPIO_WritePin(GpioPin_Name pin, GpioPin_State newVal)
{
	if(newVal == 0)
	{
		GPIO_PORT[pin]->BSRRH = GPIO_PIN[pin];
	}
	else
	{
		GPIO_PORT[pin]->BSRRL = GPIO_PIN[pin];
	}
}

void GPIO_InitPin(GpioPin_Name pin)
{
  GPIO_InitTypeDef  GPIO_InitStructure;
  
  RCC_AHB1PeriphClockCmd(GPIO_CLK[pin], ENABLE);

  GPIO_InitStructure.GPIO_Pin = GPIO_PIN[pin];
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIO_PORT[pin], &GPIO_InitStructure);
}
