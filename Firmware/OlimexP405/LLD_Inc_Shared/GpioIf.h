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
 * @brief Abstract definition of GPIO pins (usually LEDs)
*/
typedef enum
{
	LED_RED = 0,
	LED_GREEN = 1,
  CAN_LBK_GPIO = 2,
  CAN_RS_GPIO = 3,
  KLINE_K = 4,
  KLINE_L = 5,
  LED_BLUE = 6,
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
