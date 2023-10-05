/*******************************************************************************
* @file    UartIf.h
* @version V1.0
* @date    30-April-2020
* @brief   Generic definitions for setup of UART.
******************************************************************************
* @attention
******************************************************************************  
*/ 

#include <stdint.h>
#include "ErrorCodes.h"
/**
* @brief  Result of UART operation
*/
typedef enum 
{
	UART_OK = 1,                   //!< Everything ok
	UART_ERROR = 2,                //!< An error occurred
	UART_INVALID_NAME = 3,         //!< name of UART is invalid for this device
	UART_RECEIVE_BUFFER_FULL = 4,  //!< Receive buffer is full
	UART_RECEIVE_BUFFER_OVER = 5,  //!< Receive buffer was overrun (KLINE ring buffer)
	UART_DATA_OK = 1,              //!< Data OK
	UART_DATA_EMPTY = 0,           //!< Data empty
}UartResult;

/**
* @brief  Init selected UART on given baudrate
* @param  baudrate: How many bauds should be this UART set on.
* @param  name: Name of UART, which we want to init
* @retval Character status:
*            - UART_OK: Selected UART was inited fine
*            - UART_ERROR: Selected UART can't be inited
*            - UART_INVALID_NAME: Selected UART does not exist
*/
ErrorCodes Uart_Enable (uint32_t baudrate);

/**
* @brief  Disable UART
* @retval Sending status
*/
ErrorCodes Uart_Disable(void);

/**
* @brief  Gets received character from internal buffer
* @param  c: pointer to store new character to
* @retval Character status:
*            - UART_DATA_OK: Character is valid inside *c_str
*            - UART_DATA_EMPTY: No character in *c
*/
ErrorCodes Uart_Getc(uint8_t* c);

/**
* @brief  Puts character to debugging UART
* @param  c: character to send over UART
* @retval UART_OK
*/
void Uart_Putc_Debug(volatile char c);

/**
* @brief  Puts character to UART
* @param  c: character to send over UART
* @retval UART_OK
*/
ErrorCodes Uart_Putc(volatile char c);

/**
* @brief  Sends array of data
* @param  DataArray: Pointer to 8-bit data array to be sent over UART
* @param  Length: Number of elements to sent in units of bytes
* @retval Sending status
*/
ErrorCodes Uart_Tx(uint8_t* DataArray, uint32_t Length);

/**
* @brief  Receive array of data from UART
* @param  DataArray: Write data into this buffer
* @param  MaxLength: Size of buffer
* @retval Amount of received bytes
*/
uint32_t Uart_Rx(uint8_t* DataArray, uint32_t MaxLength);
