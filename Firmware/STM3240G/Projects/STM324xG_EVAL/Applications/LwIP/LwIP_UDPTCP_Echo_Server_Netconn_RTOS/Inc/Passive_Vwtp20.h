/*******************************************************************************
 * @brief   Parsing of CAN messages into VWTP2.0 protocol
 ******************************************************************************
 * @attention
 ******************************************************************************  
 */ 

#include <stdio.h>
#include <stdbool.h>
#include "CanIf.h"

/**
 * @brief Try to parse CAN as VWTP2.0 protocol
 * @retval True if successfuly processed
*/
bool Passive_Vwtp20_Parse(CanMessage cmsg);
