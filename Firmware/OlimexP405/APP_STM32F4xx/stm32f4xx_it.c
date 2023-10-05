/**
*****************************************************************************
**
**  File        : stm32f4xx_it.c
**
**  Abstract    : Main Interrupt Service Routines.
**                This file provides template for all exceptions handler and
**                peripherals interrupt service routine.
**
**  Environment : Atollic TrueSTUDIO(R)
**                STMicroelectronics STM32F4xx Standard Peripherals Library
**
**  Distribution: The file is distributed “as is,” without any warranty
**                of any kind.
**
**  (c)Copyright Atollic AB.
**  You may use this file as-is or modify it according to the needs of your
**  project. Distribution of this file (unmodified or modified) is not
**  permitted. Atollic AB permit registered Atollic TrueSTUDIO(R) users the
**  rights to distribute the assembled, compiled & linked contents of this
**  file as part of an application binary file, provided that it is built
**  using the Atollic TrueSTUDIO(R) toolchain.
**
**
*****************************************************************************
*/

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_it.h"
#include "stm32f4xx.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

#ifdef USB_OTG_HS_DEDICATED_EP1_ENABLED
extern uint32_t USBD_OTG_EP1IN_ISR_Handler (USB_OTG_CORE_HANDLE *pdev);
extern uint32_t USBD_OTG_EP1OUT_ISR_Handler (USB_OTG_CORE_HANDLE *pdev);
#endif

/* Private functions ---------------------------------------------------------*/

/******************************************************************************/
/*            Cortex-M4 Processor Exceptions Handlers                         */
/******************************************************************************/

/**
  * @brief   This function handles NMI exception.
  * @param  None
  * @retval None
  */
void NMI_Handler(void)
{
//	ERROR("\n\r");
}

/**
  * @brief  This function handles Hard Fault exception.
  * @param  None
  * @retval None
  */
void HardFault_Handler(void)
{
//	ERROR("\n\r");
	/* Go to infinite loop when Hard Fault exception occurs */
	while(1) {};
}

/**
  * @brief  This function handles Memory Manage exception.
  * @param  None
  * @retval None
  */
void MemManage_Handler(void)
{
//	ERROR("\n\r");
	/* Go to infinite loop when Memory Manage exception occurs */
	while(1) {};
}

/**
  * @brief  This function handles Bus Fault exception.
  * @param  None
  * @retval None
  */
void BusFault_Handler(void)
{
//	ERROR("\n\r");
	/* Go to infinite loop when Bus Fault exception occurs */
	while(1) {};
}

/**
  * @brief  This function handles Usage Fault exception.
  * @param  None
  * @retval None
  */
void UsageFault_Handler(void)
{
//	ERROR("\n\r");
	/* Go to infinite loop when Usage Fault exception occurs */
	while(1) {};
}

/**
  * @brief  This function handles SVCall exception.
  * @param  None
  * @retval None
  *
void SVC_Handler(void)
{
}
*/
/**
  * @brief  This function handles Debug Monitor exception.
  * @param  None
  * @retval None
  */
void DebugMon_Handler(void)
{
}

/******************************************************************************/
/*                 STM32F4xx Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f4xx.s).                                               */
/******************************************************************************/

/**
  * @brief  This function handles EXTI2_IRQ Handler.
  * @param  None
  * @retval None
  */
void EXTI1_IRQHandler(void)
{

}

/**
  * @brief  This function handles EXTI4_IRQ Handler.
  * @param  None
  * @retval None
  */
void EXTI4_IRQHandler(void)
{

}


/**
  * @brief  This function handles DMA Stream interrupt request.
  * @param  None
  * @retval None
  */
#define RESERVED_MASK           (uint32_t)0x0F7D0F7D


void DMA2_Stream0_IRQHandler(void) {

}
