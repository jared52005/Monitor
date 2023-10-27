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
 * @brief Abstract definition of GPIO pins
*/
typedef enum
{
	GPIO_LED_RED,
	GPIO_LED_GREEN,
  GPIO_LED_BLUE,
  GPIO_VBAT,
  GPIO_IGN,
  GPIO_1,
  GPIO_2,
  GPIO_3,
  GPIO_4
}GpioPin_Name;

/** 
 * @brief Abtract definition of GPIO pin state
 * @note  Underscore on the end of the name is to prevent collision with STM32F1xx StdPeriph library
*/
typedef enum
{
    GPIO_PIN_RESET_ = 0,
    GPIO_PIN_SET_ = 1,
}GpioPin_State;

/**
  * @brief  Write abstractly defined pin
  * @param  pin: Pin which we want to write
  * @param  newVal: New state, wither GPIO_PIN_RESET or GPIO_PIN_SET
  */
void GPIO_WritePin(GpioPin_Name pin, GpioPin_State newVal);

/**
  * @brief  Init pin as output one
  * @param  pin: Pin which we want to init
  */
void GPIO_InitPin(GpioPin_Name pin);
