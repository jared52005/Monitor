/*******************************************************************************
 * @brief   Reading data from peripherals, pushing them through parsers and
 *          writing them into TCP buffers for sending
 ******************************************************************************
 * @attention
 ******************************************************************************  
 */ 

#include <stdio.h>
#include <stdbool.h>
#include <esp_log.h>
#include "Task_Tcp_SocketCAN.h"
#include "Task_Tcp_Wireshark_Raw.h"
#include "System_stats.h"
#include "rtos_utils.h"
#include "CanIf.h"

#include "Passive_Iso15765.h"
#include "Passive_Vwtp20.h"

//******************************************************************************
#define TAG                             "Task_CanReconstruct.c"

//- Private Methods ------------
static void ProcessCanElements(void);

/**
* @brief  Task for parsing data into CAN
*/
void Task_CanReconstruct(void* pvParameters)
{
    //Init CAN
    Can_Enable(500000, CAN_ACTIVE);
    //Create TCP server for Wireshark
    Task_Tcp_SocketCAN_Init();
    Task_Tcp_Wireshark_Raw_Init();
    for(;;)
    {
        ProcessCanElements();
    }
}

static void ProcessCanElements(void)
{
    ErrorCodes error;
    CanMessage cmsg;

    //Read CAN element from buffer
    error = Can_Rx(&cmsg);
    if(error == ERROR_DATA_EMPTY)
    {
        return;
    }

    //Add CAN element into TCP ring buffer as socket CAN (if socket CAN is connected)
    if(Stats_TCP_WS_SocketCAN_State_Get() != 0)
    {
        Task_Tcp_SocketCAN_AddNewCanMessage(cmsg);
    }
    if(Stats_TCP_WS_RAW_State_Get() != 0)
    {
        //Try to process CAN element in ISO15765 passive protocol
        if(Passive_Iso15765_Parse(cmsg) == true)
        {
            return;
        }
        //Try to process CAN element in VWTP20 passive protocol
        if(Passive_Vwtp20_Parse(cmsg) == true)
        {
            return;
        }
    }
}
