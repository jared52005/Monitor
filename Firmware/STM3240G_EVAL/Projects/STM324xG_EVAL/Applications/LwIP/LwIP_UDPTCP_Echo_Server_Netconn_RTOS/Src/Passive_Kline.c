/*******************************************************************************
 * @brief   Parsing of KLINE bytes into Key Bytes, ISO14230 or KW1281
 ******************************************************************************
 * @attention
 ******************************************************************************  
 */ 

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "rtos_utils.h"
#include "Task_Tcp_Wireshark_Raw.h"

// -- Pirvate definitions -------------------------
#define KLINE_BUFFER_SIZE 0x200

typedef enum Kline5BaudInit
{
    K5I_ConnectionPattern_55,
    K5I_Kb1,
    K5I_Kb2,
    K5I_NKb2,
    K5I_NEcuAddress,
}Kline5BaudInit;

// -- Private variables ---------------------------
uint8_t  kline_buffer[KLINE_BUFFER_SIZE];
uint8_t  kline_frame[0x110]; //Frame to be sent to Wireshark
uint32_t kline_buffer_start = 0;
uint32_t kline_buffer_end = 0;
bool     kline_buffer_overflow;

/**
 * @brief Clean current buffer
*/
void Passive_Kline_Parse_ResetFifo()
{
    kline_buffer_start = 0;
    kline_buffer_end = 0;
    kline_buffer_overflow = false;
}

/**
 * @brief Recognize 5 baud init [00 00] 55 kb1 kb2 ~kb2 addr
*/
bool Passive_Kline_SearchKeyBytes(uint32_t* start, uint32_t* end)
{
    int i;
    int length;
    //uint8_t kb1;
    uint8_t kb2;
    //uint8_t ecuAddress;
    uint8_t nKb2;
    Kline5BaudInit init_sm = K5I_ConnectionPattern_55;
    bool result = false;

    //Go through ring buffer
    for(i = kline_buffer_start; i!= kline_buffer_end; i = ++i % KLINE_BUFFER_SIZE)
    {
        switch (init_sm)
        {
        case K5I_ConnectionPattern_55:
            if(kline_buffer[i] == 0x55)
            {
                *start = i;
                length = 1;
                init_sm = K5I_Kb1;
            }
            break;
        case K5I_Kb1:
            //kb1 = kline_buffer[i];
            init_sm = K5I_Kb2;
            length++;
            break;
        case K5I_Kb2:
            kb2 = kline_buffer[i];
            init_sm = K5I_NKb2;
            length++;
            break;
        case K5I_NKb2:
            nKb2 = (uint8_t)(~kline_buffer[i]);
            length++;
            if(nKb2 == kb2 && length == 4)
            {
                init_sm = K5I_NEcuAddress;
            }
            else
            {
                //Reset SM
                init_sm = K5I_ConnectionPattern_55;
            }
            break;
        case K5I_NEcuAddress:
            *end = i + 1;
            result = true;
            break;
        default:
            break;
        }
    }

    return result;
}

void Passive_Kline_Dequeue(uint32_t start, uint32_t end)
{
    int i;
    int framePos;
    //Dequeue crap
    if(kline_buffer_start != start)
    {
        printf("KLINE crap bytes: ");
        for(i = kline_buffer_start; i!= start; i = ++i % KLINE_BUFFER_SIZE)
        {
            printf("%x ", kline_buffer[i]);
            kline_buffer[i] = 0x00;
            kline_buffer_start++;
            kline_buffer_start = kline_buffer_start % KLINE_BUFFER_SIZE;
        }
        printf("\n");
    }

    //Dequeue data
    framePos = 0;
    printf("KLINE Data: ");
    for(i = start; i!= end; i = ++i % KLINE_BUFFER_SIZE)
    {
        kline_frame[framePos] = kline_buffer[i];
        printf("%x ", kline_buffer[i]);
        kline_buffer[i] = 0x00;
        kline_buffer_start++;
        framePos++;
        kline_buffer_start = kline_buffer_start % KLINE_BUFFER_SIZE;
        //Task_Tcp_Wireshark_Raw_AddNewRawMessage(kline_frame, framePos, 0x00, GetTime_ms(), Raw_ISO14230);
    }
    printf("\n");
}

/**
 * @brief Try to parse KLINE bytes. Result can be Key Bytes, ISO14230 or KW1281
 * @retval True if successfuly processed
*/
bool Passive_Kline_Parse(uint8_t c)
{
    
    uint32_t start;
    uint32_t end;
    //Write data into ring buffer
	  kline_buffer[kline_buffer_end] = c;
    kline_buffer_end++;
    kline_buffer_end = kline_buffer_end % KLINE_BUFFER_SIZE;
    //Check if overflow
    if(kline_buffer_start == kline_buffer_end)
    {
        kline_buffer_overflow = true;
    }
    if(kline_buffer_overflow == true)
    {
        //Reset buffer and return
        printf("KLINE buffer overflow");
        Passive_Kline_Parse_ResetFifo();
        return false;
    }

    //Process data in the buffer
    if(Passive_Kline_SearchKeyBytes(&start, &end))
    {
        Passive_Kline_Dequeue(start, end);
    }
    return true;
}
