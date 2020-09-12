/*******************************************************************************
* @brief   Implementation of basic operations with CAN peripheral
******************************************************************************
* @attention
******************************************************************************  
*/ 

#ifndef UARTIF_H
#define UARTIF_H

#include <stdint.h>
#include <stdbool.h>
#include "ErrorCodes.h"

#define UART_BUFFER_ITEMS 512

/**
* @brief  Receive one CAN message from buffer
* @param  msg: Structure where message is going to be copied
* @retval ERROR_OK: Received data are written in provided variables
*         ERROR_DATA_EMPTY: In buffer are no data available
*         ERROR_DATA_OVERFLOW: No data in buffer because pointers were reset
*/
ErrorCodes Uart_Rx(uint8_t *c);

/**
* @brief  Return amount of CAN messages in a buffer
* @param  count: Variable into which received ID will be written
* @retval ERROR_CAN_OK: Count has amount of received messages
*/
ErrorCodes Uart_Rx_GetCount(uint32_t* count);

/**
* @brief  Enable CAN peripheral
* @param  baudrate: bits per seconds which we want to set UART peripheral on
* @retval ERROR_OK: Setup OK
*/
ErrorCodes Uart_Enable(uint32_t baudrate);

/**
* @brief  Disable UART peripheral
*/
ErrorCodes Uart_Disable(void);
#endif
