/*******************************************************************************
 * @brief   Parsing of CAN messages into VWTP2.0 protocol
 ******************************************************************************
 * @attention
 ******************************************************************************  
 */ 
#include "string.h"
#include "Passive_Vwtp20.h"
#include "Task_Tcp_Wireshark_Raw.h"
#include <esp_log.h>

// -- Private defintions
typedef enum TCPI1
{
    // Last message of consecutive frame. 
    // ACK not expected
    TCPI1_CFrame_LastMessageNoAck = 0x30,
    // 2x - Consecutive frame.
    // ACK not expected
    TCPI1_CFrame_Flow = 0x20,
    // 1x - Last message of consecutive frame.
    // Expected ACK"
    TCPI1_CFrame_LastMessageAck = 0x10,
    // 0x - Block size has been reached, send ACK that RX is still OK.
    TCPI1_CFrame_BlockSizeReachedAck = 0x00,
    // Bx - Ack, "X" is SEQ
    TCPI1_Ack = 0xB0,
    // A0 - Paramters request
    TCPI1_Connection_ParamRequest = 0xA0,
    // A1 - Parameters response. Returning paramters from A0 or A3
    TCPI1_Connection_ParamResponse = 0xA1,
    // A3 - Channel test (keep alive)
    TCPI1_Connection_Test = 0xA3,
    // A4 - Break; Receiver discards all data since last ACK
    TCPI1_Connection_Break = 0xA4,
    // A8	Disconect
    TCPI1_Connection_Disconnect = 0xA8,
    // Unkown TCPI, used as deafult one
    TCPI1_Unkown,
}TCPI1;

// -- Private variables
static uint32_t testerId = 0xFFFFFFFF;
static uint32_t ecuId = 0xFFFFFFFF;
static uint8_t  tp20_frame[0x100];
static uint32_t tp20_frame_count;
static bool flag_DatagramReceived= false;
static bool flag_expectingAck = false;

#define TAG "Passive_Vwtp20.c"

/**
 * @brief Method is checking for 0x200~0x2FF CAN IDs
*/
bool Passive_Vwtp20_BroadcastChannel(CanMessage msg)
{
    //Only Messages in brodcast channel 0x200 ~ 0x2FF and DLC == 7
    if(msg.Id > 0x300 || (msg.Id & 0xF00) != 0x200 || msg.Dlc != 7)
    {
        return false;
    }
    uint8_t ecuAddress = msg.Frame[0];
    uint8_t opcode = msg.Frame[1];
    uint16_t txid = *((uint16_t*)(msg.Frame + 2));
    uint16_t rxid = *((uint16_t*)(msg.Frame + 4));
    //uint8_t appType = msg.Frame[6];
    ESP_LOGI(TAG, "ECU at address %x. TXID={%x} / RXID={%x}\n", ecuAddress, txid, rxid);

    if (msg.Id == 0x200)
    {
        //Request to ECU on brodacst channel
        ecuId = 0xFFFFFFFF;
        testerId = 0xFFFFFFFF;
    }
    else
    {
        //Response from ECU - Only react on positive response
        if(opcode == 0xD0)
        {
            ecuId = rxid;
            testerId = txid;
            tp20_frame_count = 0;
            flag_DatagramReceived = false;
        }
    }
    return true;
}

bool Passive_Vwtp20_UnicastChannel(CanMessage msg)
{
	int rtcpi;
	//int seq;
    int id = msg.Id;
    if (id != testerId && id != ecuId)
    {
        return false;
    }

    //Parse TCPI byte
    rtcpi = msg.Frame[0];
    //seq = rtcpi & 0xF;
    TCPI1 tcpi = TCPI1_Unkown;
    if ((rtcpi & 0xF0) == 0xA0)
    {
        //Use whole TCPI for parsing - Ax are constants
        tcpi = (TCPI1)rtcpi;
    }
    else
    {
        //Use only first 4 bits for parsing, which are constants
        tcpi = (TCPI1)(rtcpi & 0xF0);
    }          
    switch (tcpi)
    {
        case TCPI1_CFrame_LastMessageNoAck:
            //_datagram.AddRange(msg.Data);
            memcpy(tp20_frame + tp20_frame_count, msg.Frame, msg.Dlc);
            tp20_frame_count += msg.Dlc;
            flag_DatagramReceived = true;
            flag_expectingAck = false;
            break;
        case TCPI1_CFrame_Flow:
            //Received one frame from several frames
            //_datagram.AddRange(msg.Data.Skip(1));
            memcpy(tp20_frame + tp20_frame_count, msg.Frame + 1, msg.Dlc - 1);
            tp20_frame_count += msg.Dlc - 1;
            break;
        case TCPI1_CFrame_LastMessageAck:
            //Received last message of block expecting ACK
            //_datagram.AddRange(msg.Data.Skip(1));
            memcpy(tp20_frame + tp20_frame_count, msg.Frame + 1, msg.Dlc - 1);
            tp20_frame_count += msg.Dlc - 1;
            flag_DatagramReceived = true;
            flag_expectingAck = true;
            break;
        case TCPI1_CFrame_BlockSizeReachedAck:
            //Block has ended, expecting ACK
            //_datagram.AddRange(msg.Frame.Skip(1));
            memcpy(tp20_frame + tp20_frame_count, msg.Frame + 1, msg.Dlc - 1);
            tp20_frame_count += msg.Dlc - 1;
            flag_expectingAck = true;
            break;
        case TCPI1_Ack:
            //Datagram received
            flag_expectingAck = false;
            break;
        case TCPI1_Connection_Break:
        case TCPI1_Connection_Disconnect:
            if(tcpi == TCPI1_Connection_Disconnect)
            {
                //Reset IDs
                ecuId = 0xFFFFFFFF;
                testerId = 0xFFFFFFFF;
            }
            break;
        default:
            break;
    }

    if(flag_DatagramReceived == true)
    {
        //Check minimal length
        if (tp20_frame_count < 3)
        {
            //Too small.
            ESP_LOGE(TAG, "TP20 Datagram is smaller than 3 bytes.\n");
        }
        //Validate datagram header
        else
        {
            int dLength = tp20_frame[0] << 8 | tp20_frame[1];
            if (dLength != tp20_frame_count - 2)
            {
                //Now there is some kind of special case used for errors (maybe)
                if ((dLength ^ 0x8000) == tp20_frame_count - 2)
                {
                    tp20_frame_count = tp20_frame_count - 2;
                    Task_Tcp_Wireshark_Raw_AddNewRawMessage(tp20_frame + 2, tp20_frame_count, msg.Id, msg.Timestamp, Raw_VWTP20);
                    ESP_LOGW(TAG, "TP20 Datagram header starts on 0x8000\n");
                }
                else
                {
                    //Datagram seems to have invalid header
                    ESP_LOGE(TAG, "TP20 Datagram has invalid header.\n");
                }
            }
            else
            {
                //Seems fine
                tp20_frame_count = tp20_frame_count - 2;
                Task_Tcp_Wireshark_Raw_AddNewRawMessage(tp20_frame + 2, tp20_frame_count, msg.Id, msg.Timestamp, Raw_VWTP20);
            }
        }
        tp20_frame_count = 0;
        flag_DatagramReceived = false;
    }
    return true;
}

/**
 * @brief Parse incomming messages into VWTP2.0
*/
bool Passive_Vwtp20_Parse(CanMessage cmsg)
{
    if(Passive_Vwtp20_BroadcastChannel(cmsg) == true)
    {
        return true;
    }
    if(Passive_Vwtp20_UnicastChannel(cmsg) == true)
    {
        return true;
    }
    return false;
}
