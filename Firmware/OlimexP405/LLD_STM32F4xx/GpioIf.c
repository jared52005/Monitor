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

#ifdef STM32F40GEVAL
#define GPIOn                               4
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
#define GPIOn                               6
#define GPIO_VBAT_PIN                       GPIO_Pin_5
#define GPIO_VBAT_GPIO_PORT                 GPIOB
#define GPIO_VBAT_GPIO_CLK                  RCC_AHB1Periph_GPIOB

#define GPIO_IGN_PIN                        GPIO_Pin_2
#define GPIO_IGN_GPIO_PORT                  GPIOB
#define GPIO_IGN_GPIO_CLK                   RCC_AHB1Periph_GPIOB

#define GPIO_1_PIN                          GPIO_Pin_8
#define GPIO_1_GPIO_PORT                    GPIOA
#define GPIO_1_GPIO_CLK                     RCC_AHB1Periph_GPIOA

#define GPIO_2_PIN                          GPIO_Pin_1
#define GPIO_2_GPIO_PORT                    GPIOA
#define GPIO_2_GPIO_CLK                     RCC_AHB1Periph_GPIOA

#define GPIO_3_PIN                          GPIO_Pin_8
#define GPIO_3_GPIO_PORT                    GPIOC
#define GPIO_3_GPIO_CLK                     RCC_AHB1Periph_GPIOC

#define GPIO_4_PIN                          GPIO_Pin_9
#define GPIO_4_GPIO_PORT                    GPIOC
#define GPIO_4_GPIO_CLK                     RCC_AHB1Periph_GPIOC
/* Private variables --------------------------------------------------------*/

GPIO_TypeDef* GPIO_PORT[GPIOn] = {
	GPIO_VBAT_GPIO_PORT,
	GPIO_IGN_GPIO_PORT,
	GPIO_1_GPIO_PORT,
	GPIO_2_GPIO_PORT,
	GPIO_3_GPIO_PORT,
	GPIO_4_GPIO_PORT,
};
const uint16_t GPIO_PIN[GPIOn] = {
	GPIO_VBAT_PIN,
	GPIO_IGN_PIN,
	GPIO_1_PIN,
	GPIO_2_PIN,
	GPIO_3_PIN,
	GPIO_4_PIN,
};
const uint32_t GPIO_CLK[GPIOn] = {
	GPIO_VBAT_GPIO_CLK,
	GPIO_IGN_GPIO_CLK,
	GPIO_1_GPIO_CLK,
	GPIO_2_GPIO_CLK,
	GPIO_3_GPIO_CLK,
	GPIO_4_GPIO_CLK,
};

/**
 * @brief Search for pin, can be different for different boards
 * @retval Returns -1 if pin is not defined. Returns position of pin in GPIO_PORT, GPIO_PIN, GPIO_CLK arrays
*/
int32_t GPIO_GetPinPosition(GpioPin_Name pin)
{
	int32_t pos;
	switch (pin)
	{
	case GPIO_VBAT:
		pos = 0;
		break;
	case GPIO_IGN:
		pos = 1;
		break;
  	case GPIO_1:
		pos = 2;
		break;
  	case GPIO_2:
		pos = 3;
		break;
  	case GPIO_3:
		pos = 4;
		break;
  	case GPIO_4:
		pos = 5;
		break;
	default:
		pos = -1;
		break;
	}
	return pos;
}
#endif

void GPIO_WritePin(GpioPin_Name pin, GpioPin_State newVal)
{
	int32_t pinPos = GPIO_GetPinPosition(pin);
  	if(pinPos == -1)
  	{
		return;
  	}
	if(newVal == 0)
	{
		GPIO_PORT[pinPos]->BSRRH = GPIO_PIN[pinPos];
	}
	else
	{
		GPIO_PORT[pinPos]->BSRRL = GPIO_PIN[pinPos];
	}
}

/**
 * @brief Initiate pin as output pin
*/
void GPIO_InitPin(GpioPin_Name pin)
{
  GPIO_InitTypeDef  GPIO_InitStructure;

  int32_t pinPos = GPIO_GetPinPosition(pin);
  if(pinPos == -1)
  {
	return;
  }

  RCC_AHB1PeriphClockCmd(GPIO_CLK[pinPos], ENABLE);

  GPIO_InitStructure.GPIO_Pin = GPIO_PIN[pinPos];
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIO_PORT[pinPos], &GPIO_InitStructure);
}
