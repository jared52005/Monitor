/*******************************************************************************
 * @brief   Reading data from peripherals, pushing them through parsers and
 *          writing them into TCP buffers for sending
 ******************************************************************************
 * @attention
 ******************************************************************************  
 */ 

#include <stdio.h>
#include <stdbool.h>
#include "main.h"
#include "System_stats.h"
#include "rtos_utils.h"
#include "CanIf.h"
//******************************************************************************

//- Private Variables ------------
static void ProcessCanElements(void);

/**
* @brief  Task for parsing data into CAN
*/
void Task_Hub(void const* pvParameters)
{
    ErrorCodes error;
    error = Can_Enable(500000, CAN_ACTIVE);
    printf("CAN Setup result: %d\n", error);
    //Init CAN
    for(;;)
    {
        ProcessCanElements();
        osDelay(20);
    }
}

static void ProcessCanElements(void)
{
	uint32_t pduElements;
    ErrorCodes error;
    CanMessage cmsg;
    do
    {
        error = Can_Rx_GetCount(&pduElements);
        if(error == ERROR_DATA_OVERFLOW)
        {
            printf("ERROR: CAN Buffer Overflow\n");
        }
        else if(pduElements > 0)
        {
            //Read CAN element from buffer
            error = Can_Rx(&cmsg);
            if(error == ERROR_DATA_EMPTY)
            {
                continue;
            }

            //Add CAN element into TCP ring buffer as socket CAN (if socket CAN is connected)
            if(Stats_TCP_WS_SocketCAN_State_Get() != 0)
            {
                Task_Tcp_Wireshark_SocketCAN_AddNewCanMessage(cmsg);
            }
            
            //Try to process CAN element in VWTP20 passive protocol
            //Try to process CAN element in ISO15765 passive protocol
        }
    }
    while(pduElements > 0);
}
