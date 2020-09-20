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
//- Private Definitions --------
#define UART_BAUDRATES_COUNT 2

//- Private Methods ------------
static void ProcessCanElements(void);
static void ProcessKlineElements(void);

//- Private Variables ----------
static bool uartBaudrateChange_requested;
static int  uartBaudrateChange_selector;
static uint32_t uartBaudrates[] = {10400, 9600};

/**
 * @brief  User event to change baudrate in Task_Hub thread
*/
void Task_Hub_Uart_ChangeBaudrateRequest(void)
{
    uartBaudrateChange_requested = true;
}

/**
* @brief  Task for parsing data into CAN
*/
void Task_Hub(void const* pvParameters)
{
    ErrorCodes error;
    //Init CAN
    error = Can_Enable(500000, CAN_ACTIVE);
    //printf("CAN Setup result: %d\n", error);
    //request init UART
    uartBaudrateChange_requested = true;
    uartBaudrateChange_selector = 0;
    for(;;)
    {
        if(uartBaudrateChange_requested == true)
        {
            error = Uart_Enable(uartBaudrates[uartBaudrateChange_selector]);
            printf("UART default baudrate changed to: %d; Setup result: %d\n", uartBaudrates[uartBaudrateChange_selector], error);
            uartBaudrateChange_selector++;
            if(uartBaudrateChange_selector >= UART_BAUDRATES_COUNT)
            {
                uartBaudrateChange_selector = 0;
            }
            uartBaudrateChange_requested = false;
        }
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
    Passive_Kline_UpdateState();
    do
    {
        error = Uart_Rx_GetCount(&pduElements);
        if(error == ERROR_DATA_OVERFLOW)
        {
            printf("ERROR: UART Buffer Overflow\n");
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
