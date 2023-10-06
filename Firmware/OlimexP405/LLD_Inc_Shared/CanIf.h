/*******************************************************************************
* @file    CanIf.h
* @version V1.0
* @date    28-April-2020
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
#define CAN_BUFFER_ITEMS 128

typedef enum
{
    CAN_TYPE_DATA,   //Data frame
    CAN_TYPE_REMOTE, //Remote frame
}CanMessageType;

typedef enum
{
    CAN_ID_TYPE_STD, //Standard frame
    CAN_ID_TYPE_EXT, //Extended frame
}CanIdType;

/**
* @brief  One row of CAN message FIFO buffer
*/
typedef struct CanMessage
{
    CanMessageType RTR;
    CanIdType ID_Type;
	uint8_t   Frame[8]; //1-8 received bytes in CAN message
	uint8_t   Dlc;      //Length of received frame
	uint32_t  Id;       //ID of received frame
    uint64_t  Timestamp; //Timestamp in miliseconds
}CanMessage;

/**
* @brief CAN can work in active or passive mode
*/
typedef enum
{
	CAN_ACTIVE = 0,   //Our CAN peripheral is activelly ACKing traffic on CAN bus
	CAN_PASSIVE = 1,  //Our CAN peripheral is not ACKing traffic on CAN bus and is able only to receive
	CAN_LOOPBACK = 2, //Our CAN peripheral is working in loopback mode
}CanMode;

typedef enum {
    CAN_BITRATE_10K,
    CAN_BITRATE_20K,
    CAN_BITRATE_50K,
    CAN_BITRATE_100K,
    CAN_BITRATE_125K,
    CAN_BITRATE_250K,
    CAN_BITRATE_500K,
    CAN_BITRATE_750K,
    CAN_BITRATE_1000K,
}can_bitrate;

/**
* @brief  Receive one CAN message from buffer
* @param  canMsg: Strucutre, where received CAN message will be written
* @retval CAN_ERROR_OK: Received data are written in provided variables
*         CAN_ERROR_DATA_EMPTY: In buffer are no data available
*         CAN_ERROR_DATA_OVERFLOW: No data in buffer because pointers were reset
*/
ErrorCodes Can_Rx(CanMessage *canMsg);

/**
* @brief  Transmit message onto CAN bus
* @param  canMsg: Strucutre, where transmitted CAN message is written
* @retval CAN_ERROR_OK: Data were transmitted on CAN
*         CAN_ERROR_TOO_BIG: We are trying to transmit more than 8 bytes of data
*         CAN_ERROR_NOT_ENABLED: CAN peripheral is not enabled
*         CAN_ERROR_IS_PASSIVE: Can't transmit on CAN when in passive mode
*/
ErrorCodes Can_Tx(CanMessage *canMsg);

/**
* @brief  Check state of last transmitted messsage
* @retval CAN_ERROR_TX_FAILED  = Last message has failed to be sent
					CAN_ERROR_TX_PENDING = Last message is still waiting to be sent
					CAN_ERROR_TX_SUCCESS = Last message was successfully sent
*/
ErrorCodes Can_Tx_State(void);

/**
* @brief  Cancel transmission of last message
* @retval CAN_ERROR_OK = Transmit of CAN message was cancelled
*/
ErrorCodes Can_Tx_Cancel(void);

/**
 * @brief Return last error of CAN transmission
 * @retval 0: No Error
 *         1: Stuff Error
 *         2: Form Error
 *         3: Acknowledgment Error
 *         4: Bit recessive Error
 *         5: Bit dominant Error
 *         6: CRC Error
 *         7: Set by software
 */
ErrorCodes Can_Tx_LastError(void);

/**
* @brief  Enable CAN peripheral
* @retval CAN_ERROR_OK: Setup OK
*/
ErrorCodes Can_Enable(void);

/**
 * @brief  Set Baudrate of CAN peripheral
 * @note   Baudarate must be set before calling Can_Enable, otherwise is ignored
 * @param  baudrate: Only predefined baudrates are supported
 */
ErrorCodes Can_Baudrate_Set(can_bitrate baudrate);

/**
 * @brief  Set CAN mode (Passive or active)
 * @note   CAN mode must be set before calling Can_Enable, otherwise is ignored
*/
ErrorCodes Can_Mode_Set(CanMode mode);

/**
* @brief  Disable CAN peripheral
*/
ErrorCodes Can_Disable(void);
/**
 * @brief Set Mask/Filter
*/
void Can_Mask(uint32_t mask);
/**
 * @brief Set Mask/Filter
*/
void Can_Filter(uint32_t filter);
#endif
