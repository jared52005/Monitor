/*******************************************************************************
  * @brief   Module for sending data into Wireshark via TCP Socket
 ******************************************************************************
 * @attention
 ******************************************************************************  
 */ 

#include "CanIf.h"

/**
* @brief  Setup Wireshark Socket CAN socket
*/
void Task_Tcp_SocketCAN_Init(void);

/**
 * Adds new CAN message into a queue for sending
*/
void Task_Tcp_SocketCAN_AddNewCanMessage(CanMessage cmsg);
