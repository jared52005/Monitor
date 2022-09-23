/*******************************************************************************
 * @brief   Object is caching characters from printf() output and when \n is found
 *          or MAX_PRINTF_BUFFER size exceeded, it will send data on Wireshark Raw
 ******************************************************************************
 * @attention
 ******************************************************************************  
 */ 

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

/**
 * @brief Add new character into a buffer
 */
void Passive_Printf_Add(uint8_t c);
