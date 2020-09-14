/*******************************************************************************
* @brief   Implementation of basic operations with UART peripheral
******************************************************************************
* @attention
******************************************************************************  
*/ 
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "ErrorCodes.h"
#include "UartIf.h"
#include "System_stats.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_uart.h"
//***********************************************
#include "rtos_utils.h"

/* Private definitions -------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart;

static int uartFifo_readPtr = 0;  //Pointer where we are starting with reading
static int uartFifo_writePtr = 0; //Pointer where we are starting with writing
static bool flagOverflow = false; //Overflow flag
static uint8_t uartMessageFifo[UART_BUFFER_ITEMS]; //FIFO buffer with received CAN messages for user
static uint32_t uartBaudrate;
static uint8_t rxByte;

/* Private methods -----------------------------------------------------------*/
static ErrorCodes Uart_ResetFifo(void);

/**
* @brief  Return amount of UART messages in a buffer
* @retval How many messages is in CAN buffer ready for receving
*/
ErrorCodes Uart_Rx_GetCount(uint32_t* count)
{
	//Check if buffer has overflown
	if (flagOverflow == true)
	{
		*count = 0;
		Uart_ResetFifo();
		return ERROR_DATA_OVERFLOW;
	}

	//this setup is being used as a ring buffer
	if(uartFifo_readPtr > uartFifo_writePtr)
	{
		//If read data has higher index than write data
		//this means that write data index has jumped on start
		*count = UART_BUFFER_ITEMS - uartFifo_readPtr + uartFifo_writePtr;
	}
	else
	{
		*count = uartFifo_writePtr - uartFifo_readPtr;
	}
	return ERROR_OK;
}

/**
* @brief  Receive one CAN message from buffer
* @param  msg: Structure where message is going to be copied
* @retval ERROR_OK: Received data are written in provided variables
*         ERROR_DATA_EMPTY: In buffer are no data available
*         ERROR_DATA_OVERFLOW: No data in buffer because pointers were reset
*/
ErrorCodes Uart_Rx(uint8_t *c)
{
	//If buffer has overflow, reset pointers in FIFO
	if (flagOverflow == true)
	{
		Uart_ResetFifo();
		return ERROR_DATA_OVERFLOW;
	}

	//If pointers are same before reading and without buffer overflow flag, then buffer is empty
	if (uartFifo_writePtr == uartFifo_readPtr)
	{
		return ERROR_DATA_EMPTY;
	}

	//Read next message from buffer and copy it
	*c = uartMessageFifo[uartFifo_readPtr];

	//Move read pointer, if we are on the end of ring buffer, reset pointer
	uartFifo_readPtr++;
	if (uartFifo_readPtr >= UART_BUFFER_ITEMS)
	{
		uartFifo_readPtr = 0;
	}

	return ERROR_OK;
}

ErrorCodes Uart_ResetFifo()
{
	uartFifo_readPtr = 0;
	uartFifo_writePtr = 0;
	flagOverflow = false;
	return ERROR_OK;
}
/**
* @brief  Enable CAN peripheral
* @param  baudrate: bits per seconds which we want to set CAN peripheral on. Max 1MBit/s
* @retval ERROR_OK: Setup OK
*         ERROR_GENERAL: Name of CAN peripheral which we want to work with is not supported on this device
*/
ErrorCodes Uart_Enable(uint32_t baudrate)
{
    Uart_ResetFifo();
	uartBaudrate = baudrate;
		Stats_KlineBytes_RxByteAdd(0, baudrate);

	__GPIOB_CLK_ENABLE();
    __USART3_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.Pin = GPIO_PIN_11;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    huart.Instance = USART3;
    huart.Init.BaudRate = baudrate;
    huart.Init.WordLength = UART_WORDLENGTH_8B;
    huart.Init.StopBits = UART_STOPBITS_1;
    huart.Init.Parity = UART_PARITY_NONE;
    huart.Init.Mode = UART_MODE_TX_RX;
    huart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    HAL_UART_Init(&huart);

    HAL_NVIC_SetPriority(USART3_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART3_IRQn);

    /* Enable the UART Data Register not empty Interrupt */
    if(HAL_UART_Receive_IT(&huart, &rxByte, 1) != HAL_OK)
    {
        return ERROR_GENERAL;
    }

    return ERROR_OK;
}

/**
* @brief  Disable CAN peripheral
*/
ErrorCodes Uart_Disable()
{
    return ERROR_OK;
}

void USART3_IRQHandler (void)
{   
    HAL_UART_IRQHandler(&huart);
}

/**
  * @brief  Rx Transfer completed callback
  * @param  UartHandle: UART handle
  * @retval None
  */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *UartHandle)
{
    //Write them into ring buffer on move write pointer +1
    uartMessageFifo[uartFifo_writePtr] = rxByte;
    //Start receiving again
    HAL_UART_Receive_IT(&huart, &rxByte, 1);
    uartFifo_writePtr++; //pointers are 8bit long, so they will be always in range
    if (uartFifo_writePtr >= UART_BUFFER_ITEMS)
	{
		uartFifo_writePtr = 0;
	}
    //Check if we overrun
    if(uartFifo_writePtr == uartFifo_readPtr)
    {
        //yep, we did, reset pointers
        flagOverflow = true;
        uartFifo_writePtr = 0;
        uartFifo_readPtr = 0;
    }
    Stats_KlineBytes_RxByteAdd(1, uartBaudrate);
}
