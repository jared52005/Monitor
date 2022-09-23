/*******************************************************************************
 * @brief   Init user (key) button to generate IRQ via ETXI15. Button is then
 *          linked to Task_Hub to change the baudrate.
 ******************************************************************************
 * @attention
 ******************************************************************************
 */ 

#include <stdint.h>
#include <stdio.h>
#include "stm32f4xx_hal.h"
#include "Task_Hub.h"

// ** Private Methods *********************************************************
static void EXTILine15_10_Config(void);

/**
 * @brief Init User (Key) button to EXTI15
 */
void ButtonIf_Init(void)
{
    EXTILine15_10_Config();
}

/**
  * @brief  Configures EXTI Line15 (connected to PG15 pin) in interrupt mode
  * @param  None
  * @retval None
  */
static void EXTILine15_10_Config(void)
{
  GPIO_InitTypeDef   GPIO_InitStructure;

  /* Enable GPIOG clock */
  __HAL_RCC_GPIOG_CLK_ENABLE();
  
  /* Configure PG15 pin as input floating */
  GPIO_InitStructure.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStructure.Pull = GPIO_NOPULL;
  GPIO_InitStructure.Pin = GPIO_PIN_15;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStructure);

  /* Enable and set EXTI15_10 Interrupt to the lowest priority */
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 3, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

/**
  * @brief  EXTI line detection callbacks
  * @param  GPIO_Pin: Specifies the pins connected EXTI line
  * @retval None
  */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == GPIO_PIN_15)
  {
    Task_Hub_Uart_ChangeBaudrateRequest();
  }
}
