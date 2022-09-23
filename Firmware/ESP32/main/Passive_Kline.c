/*******************************************************************************
 * @brief   Parsing of KLINE bytes into Key Bytes, ISO14230 or KW1281
 ******************************************************************************
 * @attention
 ******************************************************************************  
 */ 

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <esp_log.h>
#include "rtos_utils.h"
#include "System_stats.h"
#include "Task_Tcp_Wireshark_Raw.h"

// -- Pirvate definitions -------------------------
#define KLINE_BUFFER_SIZE 0x200
#define KLINE_LAST_ACTIVTY_DELAY 3000 //3000 ms

typedef enum Kline5BaudInit
{
    K5I_ConnectionPattern_55,
    K5I_Kb1,
    K5I_Kb2,
    K5I_NKb2,
    K5I_NEcuAddress,
}Kline5BaudInit;

typedef enum KlineBusState
{
    KBS_Idle,     //Nothing is happening on the KLINE. Automatically slips into idle after 2~3 seconds and erase buffer
    KBS_Iso14230, //Try to parse incomming data as ISO14230. Will get set after 5 baud init or used as default if 5 baud is not parsed correctly
    KBS_Kw1281,   //Try to parse incomming data as KW1281. Will ONLY happen after 5 baud init.
}KlineBusState;

typedef enum KlineKw1281
{
    Kw1281_Data,
    Kw1281_DataComplement,
    Kw1281_Etx,
}KlineKw1281;

typedef enum KlineIso14230
{
    Iso14230_Fmt,
    Iso14230_Tgt,
    Iso14230_Src,
    Iso14230_Len,
    Iso14230_Data,
    Iso14230_Cs,
}KlineIso14230;

typedef enum KlineIso14230_FrameParseResult
{
    Iso14230_Good,
    Iso14230_InvalidCs,
    Iso14230_NotEnoughData,
    Iso14230_InvalidState,
}KlineIso14230_FrameParseResult;

// -- Private definitions
#define TAG "Passive_Kline.c"

// -- Private variables ---------------------------
KlineBusState kline_bus_state = KBS_Idle; //By default parse data as ISO14230
uint8_t  kline_buffer[KLINE_BUFFER_SIZE];
uint8_t  kline_frame[0x110]; //Frame to be sent to Wireshark
uint32_t kline_buffer_end = 0;
uint32_t kline_last_activity;

void Passive_Kline_PrintBuffer(int start, int length)
{
    ESP_LOG_BUFFER_HEXDUMP(TAG, kline_buffer + start, length, ESP_LOG_INFO);
}

void Passive_Kline_UpdateState()
{
    if((kline_last_activity + KLINE_LAST_ACTIVTY_DELAY) < GetTime_ms())
    {
        if(kline_buffer_end != 0 || kline_bus_state != KBS_Idle)
        {
            if(kline_buffer_end != 0)
            {
                Passive_Kline_PrintBuffer(0, kline_buffer_end);
            }
            kline_bus_state = KBS_Idle;
            kline_buffer_end = 0;
            ESP_LOGW(TAG, "KLINE Reset back to default @ %d ms\n", GetTime_ms());
        }
    }
}

KlineIso14230_FrameParseResult Passive_Kline_SearchIso14230_Frame(uint32_t offset, uint32_t* start, uint32_t* end)
{
    int i;
    uint8_t frame_Fmt; //Expected Fmt
    uint8_t frame_Len; //Expected length of a payload
    uint32_t frame_ExpectedLength; //Length of whole frame
    uint32_t frame_FoundLength;
    uint32_t frame_Cs;
    KlineIso14230 iso14230_sm = Iso14230_Fmt;
    for(i = offset; i < kline_buffer_end; i++)
    {
        switch (iso14230_sm)
        {
        case Iso14230_Fmt:
            *start = i;
            frame_FoundLength = 1;
            frame_ExpectedLength = 1; //FMT + (CS)
            frame_Cs = kline_buffer[i];
            frame_Fmt = kline_buffer[i];

            //Check if length is in FMT
            frame_Len = frame_Fmt & 0x3F;
            if(frame_Len != 0)
            {
                frame_ExpectedLength = frame_ExpectedLength + frame_Len;
                if(frame_ExpectedLength > kline_buffer_end)
                {
                    //Not all data were received
                    return Iso14230_NotEnoughData;
                }
            }

            //Check if we have address information
            if((frame_Fmt & 0x80) == 0x80)
            {
                //Yes we do. Next state will be TGT / SRC
                frame_ExpectedLength += 2;
                iso14230_sm = Iso14230_Tgt;
            }
            else
            {
                //No we don't. Set next state by frame_Len
                if(frame_Len == 0)
                {
                    //FMT.LEN == 0, so next should be LEN byte
                    iso14230_sm = Iso14230_Len;
                }
                else
                {
                    //FMT.LEN != 0, so next should be Data byte(s)
                    iso14230_sm = Iso14230_Data;
                }
            }
            break;
        case Iso14230_Tgt:
            frame_FoundLength++;
            frame_Cs += kline_buffer[i];
            iso14230_sm = Iso14230_Src;
            break;
        case Iso14230_Src:
            frame_FoundLength++;
            frame_Cs += kline_buffer[i];
            if(frame_Len == 0)
            {
                iso14230_sm = Iso14230_Len;
            }
            else
            {
                iso14230_sm = Iso14230_Data;
            }
            break;
        case Iso14230_Len:
            frame_FoundLength++;
            frame_Cs += kline_buffer[i];
            frame_Len = kline_buffer[i];
            frame_ExpectedLength++; //LEN byte
            frame_ExpectedLength = frame_ExpectedLength + frame_Len;
            if(frame_ExpectedLength > kline_buffer_end)
            {
                //Not all data were received
                return Iso14230_NotEnoughData;
            }
            iso14230_sm = Iso14230_Data;
            break;
        case Iso14230_Data:
            frame_FoundLength++;
            frame_Cs += kline_buffer[i];
            if(frame_FoundLength == frame_ExpectedLength)
            {
                iso14230_sm = Iso14230_Cs;
            }
            break;
        case Iso14230_Cs:
            if((uint8_t)frame_Cs == kline_buffer[i])
            {
                *end = i + 1;
                return Iso14230_Good;
            }
            else
            {
                return Iso14230_InvalidCs;
            }
        default:
            return Iso14230_InvalidState;
        }
    }
    return Iso14230_NotEnoughData;
}

bool Passive_Kline_SearchIso14230(uint32_t* start, uint32_t* end)
{
    uint32_t offset;
    bool result = false;
    int frameParseResult;
    //ISO14230 packet must be at least 3 bytes long
    if(kline_buffer_end < 3)
    {
        return false;
    }
    //Search through buffer
    offset = 0;
    //for(offset = 0; offset < (buffer_length - 3); offset++)
    {
        frameParseResult = Passive_Kline_SearchIso14230_Frame(offset, start, end);
        if(frameParseResult == Iso14230_Good)
        {
            //Process frame
            result = true;
            //break;
        }
        else if(frameParseResult == Iso14230_NotEnoughData)
        {
            //Wait on more data
            result = false;
            //break;
        }
        else
        {
            kline_buffer_end = 0;
            result = false;
        }
    }

    return result;
}

bool Passive_Kline_SearchKw1281(uint32_t* start, uint32_t* end)
{
    int i;
    int expectedLength;
    bool result = false;
    int length = 0;
    int etx = 0x03;
    uint8_t data;
    uint8_t data_c;
    KlineKw1281 kw1281_sm = Kw1281_Data;
    /* 0F f0   Length
       1 fe    Block ID
       f6 9    Block Type
       b4 4b 
       5a a5 
       37 c8
       39 c6 
       30 cf 
       37 c8 
       35 ca 
       35 ca 
       31 ce 
       41 be 
       41 be 
       20 df 
       3      Last byte is ETX without a complement.
       */   

    //Go through buffer
    for(i = 0; i < kline_buffer_end; i++)
    {
        switch (kw1281_sm)
        {
        case Kw1281_Data:
            data = kline_buffer[i];
            kw1281_sm = Kw1281_DataComplement;
            if(length == 0)
            {
                *start = i;
            }
            break;
        case Kw1281_DataComplement:
            data_c = (uint8_t)(~kline_buffer[i]);
            if(data == data_c)
            {
                length++;
                if(length == 1)
                {
                    expectedLength = data;
                }
                if(length == expectedLength)
                {
                    kw1281_sm = Kw1281_Etx;
                }
                else
                {
                    kw1281_sm = Kw1281_Data;
                }
            }
            else
            {
                length = 0;
                kw1281_sm = Kw1281_Data;
            }
            break;
        case Kw1281_Etx:
            data = kline_buffer[i];
            if(data == etx)
            {
                *end = i + 1;
                result = true;
            }
            break;
        default:
            break;
        }
    }
    return result;
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
    for(i = 0; i < kline_buffer_end; i++)
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
                if(kb2 == 0x8A)
                {
                    //KW1281 is not sending back ECU address
                    kline_bus_state = KBS_Kw1281;
                    *end = i + 1;
                    result = true;
                }
                init_sm = K5I_NEcuAddress;
            }
            else
            {
                //Reset SM
                init_sm = K5I_ConnectionPattern_55;
            }
            break;
        case K5I_NEcuAddress:
            if(kb2 == 0x8F)
            {
                kline_bus_state = KBS_Iso14230;
                *end = i + 1;
                result = true;
            }
            else
            {
                ESP_LOGE(TAG, "Unknown KLINE protocol: %x\n", kb2);
            }
            break;
        default:
            break;
        }
    }

    return result;
}

/**
 * @brief Dequeue data from ring buffer, including junk data between start of buffer and start of packet
 */
void Passive_Kline_Dequeue_Iso14230(uint32_t start, uint32_t end)
{
    int i;
    int framePos;
    //Dequeue crap
    if(start != 0)
    {
        ESP_LOGW(TAG, "ISO14230 crap bytes: ");
        Passive_Kline_PrintBuffer(0, start);
    }

    //Dequeue data
    framePos = 0;
    for(i = start; i!= end; i++)
    {
        kline_frame[framePos] = kline_buffer[i];
        //printf("%x ", kline_buffer[i]);
        kline_buffer[i] = 0x00;
        framePos++;
    }
    Stats_KlineBytes_RxFrameAdd(1);
    Task_Tcp_Wireshark_Raw_AddNewRawMessage(kline_frame, framePos, 0x00, GetTime_ms(), Raw_ISO14230);
    kline_buffer_end = 0;
}

/**
 * @brief Dequeue data from ring buffer using data, nData structure, including junk data between start of buffer and start of packet
 */
void Passive_Kline_Dequeue_Kw1281(uint32_t start, uint32_t end)
{
    int i;
    int framePos;
    int length;
    //Dequeue crap
    if(start != 0)
    {
        ESP_LOGW(TAG, "KW1281 crap bytes: ");
        Passive_Kline_PrintBuffer(0, start);
    }

    //Dequeue data
    framePos = 0;
    length = 0;
    for(i = start; i!= end; i++)
    {
        //Write only even positions. Odd positions are complements
        if(length % 2 == 0)
        {
            kline_frame[framePos] = kline_buffer[i];
            //printf("%x ", kline_buffer[i]);
            //Check for End Communication command
            if((framePos == 2) && (kline_frame[framePos] == 0x06))
            {
                //If correct, reset kline bus status
                ESP_LOGI(TAG, "KLINE bus reset from KW1281 command\n");
                kline_bus_state = KBS_Idle;
            }
            framePos++;
        }
        kline_buffer[i] = 0x00;
        length++;
    }
    Stats_KlineBytes_RxFrameAdd(1);
    Task_Tcp_Wireshark_Raw_AddNewRawMessage(kline_frame, framePos, 0x00, GetTime_ms(), Raw_KW1281);
    kline_buffer_end = 0;
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
    //Check if overflow
    if(kline_buffer_end == KLINE_BUFFER_SIZE)
    {
        ESP_LOGW(TAG, "KLINE buffer overflow: ");
        Passive_Kline_PrintBuffer(0, kline_buffer_end - 1);
        kline_buffer_end = 0;
        kline_bus_state = KBS_Idle;
        return false;
    }

    kline_last_activity = GetTime_ms();

    //Process data in the buffer
    switch (kline_bus_state)
    {
    case KBS_Idle:
        //Try to parse keybytes
        if(Passive_Kline_SearchKeyBytes(&start, &end))
        {
            Passive_Kline_Dequeue_Iso14230(start, end);
        }
        else
        {
            //Try to parse it as ISO14230
            if(Passive_Kline_SearchIso14230(&start, &end))
            {
                //Success, switch state of bus into ISO14230
                kline_bus_state = KBS_Iso14230;
                Passive_Kline_Dequeue_Iso14230(start, end);
            }    
        }
        break;
    case KBS_Iso14230:
        if(Passive_Kline_SearchIso14230(&start, &end))
        {
            Passive_Kline_Dequeue_Iso14230(start, end);
        }
        break;
    case KBS_Kw1281:
        if(Passive_Kline_SearchKw1281(&start, &end))
        {
            Passive_Kline_Dequeue_Kw1281(start, end);
        }
        break;
    default:
        ESP_LOGE(TAG, "KLINE: Unknown protocol\n");
        break;
    }
    return true;
}
