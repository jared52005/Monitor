/*******************************************************************************
  * @brief   Module for sending data into Wireshark via TCP Socket
 ******************************************************************************
 * @attention
 ******************************************************************************  
 */ 
#include <stdint.h>
typedef enum RawMessageType
{
  Raw_ISO14230 = 0x91,
  Raw_KW1281 = 0x92,
  Raw_VWTP20 = 0x93,
  Raw_ISO15765 = 0x94,
  
  Raw_Debug = 0xFA,    //For Debug information (i.e. SWO output)
  Raw_Warning = 0xFB,
  Raw_Error = 0xFC,
}RawMessageType;

/**
* @brief  One row of RAW message in FIFO buffer
*/
typedef struct RawMessage
{
	uint8_t*  Frame;
	uint32_t  Length;      //Length of received frame
	uint32_t  Id;          //ID of received frame
	uint32_t  Timestamp;
  RawMessageType MessageType;
}RawMessage;


/**
* @brief  Setup Wireshark Socket CAN socket
*/
void Task_Tcp_Wireshark_Raw_Init(void);

/**
 * Adds new CAN message into a queue for sending
*/
void Task_Tcp_Wireshark_Raw_AddNewRawMessage(uint8_t* frame, uint32_t length, uint32_t id, uint32_t timestamp, RawMessageType msgType);
