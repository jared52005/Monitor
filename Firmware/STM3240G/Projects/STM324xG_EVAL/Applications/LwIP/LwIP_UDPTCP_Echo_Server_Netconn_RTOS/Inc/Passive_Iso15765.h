/*******************************************************************************
 * @brief   Parsing of CAN messages into ISO15765 protocol
 ******************************************************************************
 * @attention
 ******************************************************************************  
 */ 

#include <stdio.h>
#include <stdbool.h>
#include "CanIf.h"

/**
 * @brief Try to parse CAN as ISO15765 protocol
 * @retval True if successfuly processed
*/
bool Passive_Iso15765_Parse(CanMessage cmsg);
