/*******************************************************************************
 * @brief   Parsing of CAN messages into ISO15765 protocol
 ******************************************************************************
 * @attention
 ******************************************************************************  
 */ 

#include "Passive_Iso15765.h"
#include "Task_Tcp_Wireshark_Raw.h"

// -- Private variables
static uint32_t valid_CanIds[] = {0x700, 0x7E0, 0x7E8, 0x7E1, 0x7E9 };
static uint32_t valid_CanIds_Count = 5;
static uint8_t  iso15765_frame[4096];
static uint32_t iso15765_frame_position = 0;
static uint32_t iso15765_frame_expectedLength = 0;
static uint32_t iso15765_frame_expectedSN;

bool Passive_Iso15765_Contains(uint32_t id)
{
    int i;
    for(i = 0; i<valid_CanIds_Count;i++)
    {
        if(valid_CanIds[i] == id)
        {
            return true;
        }
    }
    return false;
}
void Passive_Iso15765_VerifyPreviousDatagram()
{
    if (iso15765_frame_position != 0)
    {
        printf("We are trying to process another datagram, even that we still have datagram with length 0x%x bytes in buffer", iso15765_frame_position);
        printf("Datagram is still missing 0x%x bytes to be complete", iso15765_frame_expectedLength);        
        iso15765_frame_position = 0;
    }
}

void Passive_Iso15765_SingleFrame(CanMessage cmsg)
{
    int i;
    Passive_Iso15765_VerifyPreviousDatagram();
    int length = cmsg.Frame[0] & 0xF;
    if (length > cmsg.Dlc - 1)
    {
        return;
    }

    for (i = 1; i < length + 1; i++)
    {
        iso15765_frame[iso15765_frame_position] = cmsg.Frame[i];
        iso15765_frame_position++;
    }
    Task_Tcp_Wireshark_Raw_AddNewRawMessage(iso15765_frame, iso15765_frame_position, cmsg.Id, cmsg.Timestamp, Raw_ISO15765);
    iso15765_frame_position = 0;
}

void Passive_Iso15765_FirstFrame(CanMessage cmsg)
{
    int i;
    Passive_Iso15765_VerifyPreviousDatagram();
    iso15765_frame_expectedLength = ((cmsg.Frame[0] & 0xF) << 8) | cmsg.Frame[1];
    iso15765_frame_expectedSN = 1; //Always starting on 1
    for (i = 2; i < cmsg.Dlc; i++)
    {
        iso15765_frame[iso15765_frame_position] = cmsg.Frame[i];
        iso15765_frame_position++;
        iso15765_frame_expectedLength--;

        if (iso15765_frame_expectedLength == 0)
        {
            Task_Tcp_Wireshark_Raw_AddNewRawMessage(iso15765_frame, iso15765_frame_position, cmsg.Id, cmsg.Timestamp, Raw_ISO15765);
            iso15765_frame_position = 0;
            break;
        }
    }
}

void Passive_Iso15765_ConsequtiveFrame(CanMessage cmsg)
{
    int i;
    int receivedSN;
    if((cmsg.Frame[0] & 0xF) == iso15765_frame_expectedSN)
    {
        iso15765_frame_expectedSN++;
        iso15765_frame_expectedSN = iso15765_frame_expectedSN & 0xF;
    }
    else
    {
        receivedSN = (cmsg.Frame[0] & 0xF);
        printf("Expected S/N = 0x%x. Provided 0x%x\n", iso15765_frame_expectedSN, receivedSN);
        iso15765_frame_expectedSN = receivedSN + 1;
    }

    for (i = 1; i < cmsg.Dlc; i++)
    {
        iso15765_frame[iso15765_frame_position] = cmsg.Frame[i];
        iso15765_frame_position++;
        iso15765_frame_expectedLength--;
        
        if (iso15765_frame_expectedLength == 0)
        {
            Task_Tcp_Wireshark_Raw_AddNewRawMessage(iso15765_frame, iso15765_frame_position, cmsg.Id, cmsg.Timestamp, Raw_ISO15765);
            iso15765_frame_position = 0;
            break;
        }
    }
}

/**
 * @brief Try to parse CAN as ISO15765 protocol
 * @retval True if successfuly processed
*/
bool Passive_Iso15765_Parse(CanMessage cmsg)
{
    int NPCI;
    //Check if we have this ID on list of allowed IDs to parse
    if(Passive_Iso15765_Contains(cmsg.Id) == false)
    {
        //Use only target IDs
        return false;
    }
    if (cmsg.Dlc == 0)
    {
        //Only non-zero lengths
        return false;
    }
    NPCI = cmsg.Frame[0] >> 4;

    //Switch according the first 4 bits
    switch (NPCI)
    {
        case 0:
            Passive_Iso15765_SingleFrame(cmsg);
            break;
        case 1:
            Passive_Iso15765_FirstFrame(cmsg);
            break;
        case 2:
            Passive_Iso15765_ConsequtiveFrame(cmsg);
            break;
        case 3:
            //Do nothing with flow control frame
            break;
        default:
            printf("ISO15765: Invalid PCI byte was provided!");
            return false;
    }
    return true;
}
