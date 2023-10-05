#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "ErrorCodes.h"
#include "CanRawLoopback.h"
#include "CanIf.h"
//******************************************************************************
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "croutine.h"
#include "rtos_utils.h"
//******************************************************************************

/**
* @brief  Thread in which we are refreshing CAN state machine
*/
void CanRawLoopback_Task(void* arg)
{
	ErrorCodes canResult;
	CanMessage canRxMsg;
	int i;
    for(;;)
    {
        canResult = Can_Rx(&canRxMsg, 2);
	    if (canResult == ERROR_OK)
	    {  
			printf("CAN Raw - [");
			for(i = 0; i < canRxMsg.Dlc; i++)
			{
				printf("%x ", canRxMsg.Frame[i]);
			}
			printf("]\r\n");
			canRxMsg.Id = 0x7FF;
		    Can_Tx(&canRxMsg);
	    }
		if(canResult == ERROR_DATA_OVERFLOW)
		{
			printf("CAN Raw - Overflow!\r\n");
		}
		taskYIELD();
    }
}
