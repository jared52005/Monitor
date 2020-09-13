/*******************************************************************************
 * @brief   Parsing of KLINE bytes into Key Bytes, ISO14230 or KW1281
 ******************************************************************************
 * @attention
 ******************************************************************************  
 */ 

#include <stdio.h>
#include <stdbool.h>

/**
 * @brief Try to parse KLINE bytes. Result can be Key Bytes, ISO14230 or KW1281
 * @retval True if successfuly processed
*/
bool Passive_Kline_Parse(uint8_t c);
