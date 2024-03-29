#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "ErrorCodes.h"
#include "CanDriver.h"
#include "CanIf.h"
//******************************************************************************
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "croutine.h"
#include "rtos_utils.h"
//******************************************************************************

static QueueHandle_t canMessage_RxQueue = NULL;
static QueueHandle_t canMessage_TxQueue = NULL;
static bool _stop;
/**
* @brief  Thread in which we are refreshing CAN state machine
*/
void CanDriver_Task(void* arg)
{
	ErrorCodes canResult;
	CanMessage canRxMsg;
	CanMessage* canRxMsg_Queue;
	CanMessage* canTxMsg;
	_stop = false;
	
	//printf("CAN Driver task has started\n");
    if (Can_Enable() != ERROR_OK)
	{
        printf("ERROR: Init of CAN peripheral has failed\r\n");
    }
	//Hold 32 messages max 
  	canMessage_RxQueue = xQueueCreate( 32, sizeof( CanMessage* ) );
	canMessage_TxQueue = xQueueCreate( 32, sizeof( CanMessage* ) );
    while(_stop == false)
    {
		//Check RX messages
        canResult = Can_Rx(&canRxMsg);
	    if (canResult == ERROR_OK)
	    {
			//Copy cmsg into allocated qCanMessage and queue it
    		canRxMsg_Queue = (CanMessage*)pvPortMalloc(sizeof(CanMessage));
    		canRxMsg_Queue->Dlc = canRxMsg.Dlc;
    		canRxMsg_Queue->Id = canRxMsg.Id;
			canRxMsg_Queue->ID_Type = canRxMsg.ID_Type;
			canRxMsg_Queue->RTR = CAN_TYPE_DATA;
    		canRxMsg_Queue->Timestamp = canRxMsg.Timestamp;
    		memcpy(canRxMsg_Queue->Frame, canRxMsg.Frame, canRxMsg.Dlc);
    		xQueueSend(canMessage_RxQueue, ( void * ) &canRxMsg_Queue, (TickType_t)0);
			//print received CAN message on USB
			//printf("Received %x - %x [%x]\n", canRxMsg.Id, canRxMsg.Dlc, canRxMsg.Frame[0]);
	    }
		
		//Check TX Queue
    	if(xQueueReceive( canMessage_TxQueue, &(canTxMsg), ( TickType_t ) 0 ) == pdPASS)
    	{
			Can_Tx(canTxMsg);
      		vPortFree(canTxMsg);
    	}
		taskYIELD();
    }
	vQueueDelete(canMessage_RxQueue);
	vQueueDelete(canMessage_TxQueue);
	vTaskDelete(NULL);
}

void CanDriver_Start(void)
{
	xTaskCreate(CanDriver_Task, (const char*)"CAN Driver", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
}

void CanDriver_Stop(void)
{
	_stop = true;
}

void CanDriver_Transmit(CanMessage cmsg)
{
	CanMessage* qCanMessage;
	if(canMessage_TxQueue == NULL)
	{
    	return;
  	}
	//Copy cmsg into allocated qCanMessage and queue it
    qCanMessage = (CanMessage*)pvPortMalloc(sizeof(CanMessage));
    qCanMessage->Dlc = cmsg.Dlc;
    qCanMessage->Id = cmsg.Id;
	qCanMessage->ID_Type = cmsg.ID_Type;
    qCanMessage->Timestamp = cmsg.Timestamp;
    memcpy(qCanMessage->Frame, cmsg.Frame, cmsg.Dlc);
    xQueueSend(canMessage_TxQueue, ( void * ) &qCanMessage, (TickType_t)0);
}

bool CanDriver_Receive(CanMessage* cmsg)
{
	CanMessage* xcmsg;
	if(canMessage_TxQueue == NULL)
	{
    	return false;
  	}

    if(xQueueReceive( canMessage_RxQueue, &(xcmsg), ( TickType_t ) 0 ) == pdPASS)
    {
		//Copy allocated xcmsg into user's cmsg, and free it
		cmsg->Dlc = xcmsg->Dlc;
    	cmsg->Id = xcmsg->Id;
		cmsg->ID_Type = xcmsg->ID_Type;
    	memcpy(cmsg->Frame, xcmsg->Frame, cmsg->Dlc);
		//printf("Dequeued %x - %x [%x]", cmsg->Id, cmsg->Dlc, cmsg->Frame[0]);
      	vPortFree(xcmsg);
	  	return true;
    }
	else
	{
		return false;
	}
}
