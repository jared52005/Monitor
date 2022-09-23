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

#include "Uart.h"
#include "Passive_Kline.h"

//******************************************************************************
#define TAG     "Task_KlineReconstruct.c"

//- Private Methods ------------
static void ProcessKlineElements(void);

/**
* @brief  Task for parsing data into CAN
*/
void Task_KlineReconstruct(void* pvParameters)
{
    //Init UART
    Uart_Enable();
    //Create TCP server for Wireshark (not necessary, already done from CAN task)
    //Task_Tcp_Wireshark_Raw_Init();
    for(;;)
    {
        ProcessKlineElements();
    }
}

static void ProcessKlineElements(void)
{
    ErrorCodes error;
    uint8_t c;

    //Read UART element from buffer
    error = Uart_Rx(&c);
    if(error == ERROR_DATA_EMPTY)
    {
        return;
    }

    if(Stats_TCP_WS_RAW_State_Get() != 0)
    {
        //Try to process element in passive KLINE protocol
        Passive_Kline_Parse(c);
    }
}
