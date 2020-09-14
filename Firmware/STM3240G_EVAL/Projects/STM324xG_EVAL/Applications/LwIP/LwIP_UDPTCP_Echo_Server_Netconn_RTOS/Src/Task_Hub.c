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
#include "Task_Tcp_Wireshark_SocketCAN.h"
#include "System_stats.h"
#include "rtos_utils.h"
#include "CanIf.h"
#include "UartIf.h"

#include "Passive_Iso15765.h"
#include "Passive_Vwtp20.h"
#include "Passive_Kline.h"

//******************************************************************************

//- Private Variables ------------
static void ProcessCanElements(void);
static void ProcessKlineElements(void);

/**
* @brief  Task for parsing data into CAN
*/
void Task_Hub(void const* pvParameters)
{
    ErrorCodes error;
    //Init CAN
    error = Can_Enable(500000, CAN_ACTIVE);
    printf("CAN Setup result: %d\n", error);
    //Init UART
    error = Uart_Enable(10400);
    printf("UART Setup result: %d\n", error);
    for(;;)
    {
        ProcessCanElements();
        ProcessKlineElements();
        osDelay(20);
    }
}

static void ProcessKlineElements(void)
{
    uint8_t c;
    uint32_t pduElements;
    ErrorCodes error;
    do
    {
        error = Uart_Rx_GetCount(&pduElements);
        if(error == ERROR_DATA_OVERFLOW)
        {
            printf("ERROR: KLINE Buffer Overflow\n");
        }
        else if(pduElements > 0)
        {
            //Read CAN element from buffer
            error = Uart_Rx(&c);
            if(error == ERROR_DATA_EMPTY)
            {
                continue;
            }

            //Add CAN element into TCP ring buffer as socket CAN (if socket CAN is connected)
            if(Stats_TCP_WS_RAW_State_Get() != 0)
            {
                Passive_Kline_Parse(c);
            }
        }
    }
    while(pduElements > 0);
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
            if(Stats_TCP_WS_RAW_State_Get() != 0)
            {
                //Try to process CAN element in ISO15765 passive protocol
                if(Passive_Iso15765_Parse(cmsg) == true)
                {
                    continue;
                }
                //Try to process CAN element in VWTP20 passive protocol
                if(Passive_Vwtp20_Parse(cmsg) == true)
                {
                    continue;
                }
            }
        }
    }
    while(pduElements > 0);
}
