/*******************************************************************************
 * @brief   Object is caching characters from printf() output and when \n is found
 *          or MAX_PRINTF_BUFFER size exceeded, it will send data on Wireshark Raw
 ******************************************************************************
 * @attention
 ******************************************************************************  
 */ 

#include "Passive_Printf.h"
#include "Task_Tcp_Wireshark_Raw.h"
#include "System_stats.h"
#include "rtos_utils.h"

// ** Private defintions
#define MAX_PRINTF_BUFFER 255

// ** Private variables 
static uint8_t  printf_buffer[MAX_PRINTF_BUFFER];
static uint32_t printf_buffer_position = 0;

/**
 * @brief Add new character into a buffer
 */
void Passive_Printf_Add(uint8_t c)
{
    printf_buffer[printf_buffer_position] = c;
    printf_buffer_position++;

    //If we have exceeded amount of data or found \n chracter, send the buffer
    if(c == '\n' || printf_buffer_position >= MAX_PRINTF_BUFFER)
    {
        //Add datagram into a queue only if we are connected
        if(Stats_TCP_WS_RAW_State_Get() != 0)
        {
            Task_Tcp_Wireshark_Raw_AddNewRawMessage(printf_buffer, printf_buffer_position, 0, GetTime_ms(), Raw_Debug);
        }
        printf_buffer_position = 0;
    }
}
