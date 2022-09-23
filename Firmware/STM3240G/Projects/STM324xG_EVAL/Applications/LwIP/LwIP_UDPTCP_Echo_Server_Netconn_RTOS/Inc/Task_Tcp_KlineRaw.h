/*******************************************************************************
  * @brief   Module for sending raw KLINE bytes into TCP socket
 ******************************************************************************
 * @attention
 ******************************************************************************  
 */ 

#include <stdint.h>

/**
* @brief  Setup Wireshark Socket CAN socket
*/
void Task_Tcp_Kline_Init(void);

/**
 * Adds new CAN message into a queue for sending
*/
void Task_Tcp_Kline_AddNewByte(uint8_t c);
