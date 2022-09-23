/*******************************************************************************
* @brief   Implementation of basic operations with CAN peripheral
******************************************************************************
* @attention
******************************************************************************  
*/ 

#ifndef CANIF_H
#define CANIF_H

#include <stdint.h>
#include <stdbool.h>
#include "ErrorCodes.h"

/** 
* @brief How many messages can be stored in a CAN buffer
*        1 CAN message = 13byte [uint32_t:ID][uint8_t:DLC][uint8_t[8]:Data]
*        512 * 13 =  6656 bytes
*        CAN buffer is a structure array, which is being written as ring buffer
*/
#define CAN_BUFFER_ITEMS 512

/**
* @brief  One row of CAN message FIFO buffer
*/
typedef struct CanMessage
{
	uint8_t   Frame[8]; //1-8 received bytes in CAN message
	uint8_t   Dlc;      //Length of received frame
	uint32_t  Id;       //ID of received frame
	uint32_t  Timestamp;
}CanMessage;

/**
* @brief CAN can work in active or passive mode
*/
typedef enum
{
	CAN_ACTIVE = 0,
	CAN_PASSIVE = 1,
}CanMode;

/**
* @brief  Receive one CAN message from buffer
* @param  msg: Structure where message is going to be copied
* @retval ERROR_OK: Received data are written in provided variables
*         ERROR_DATA_EMPTY: In buffer are no data available
*         ERROR_DATA_OVERFLOW: No data in buffer because pointers were reset
*/
ErrorCodes Can_Rx(CanMessage *msg);

/**
* @brief  Return amount of CAN messages in a buffer
* @param  count: Variable into which received ID will be written
* @retval ERROR_CAN_OK: Count has amount of received messages
*         ERROR_CAN_DATA_OVERFLOW: No data in buffer because pointers were reset
*/
ErrorCodes Can_Rx_GetCount(uint32_t* count);

/**
* @brief  Enable CAN peripheral
* @param  baudrate: bits per seconds which we want to set CAN peripheral on. Max 1MBit/s
* @param  mode: We can set device into passive or active mode
* @retval ERROR_OK: Setup OK
*/
ErrorCodes Can_Enable(uint32_t baudrate, CanMode mode);

/**
* @brief  Disable CAN peripheral
*/
ErrorCodes Can_Disable(void);
#endif
