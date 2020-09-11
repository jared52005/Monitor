/*******************************************************************************
* @brief   Implementation of basic operations with CAN peripheral
******************************************************************************
* @attention
******************************************************************************  
*/ 
#include <stdint.h>
#include <stdbool.h>
#include "ErrorCodes.h"
#include "CanIf.h"
#include "System_stats.h"
#include "stm32f4xx_hal.h"
//#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_can.h"
//#include "stm32f4xx_hal_rcc.h"
//***********************************************
#include "rtos_utils.h"

/* Private definitions -------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
CAN_HandleTypeDef     hcan;

int canFifo_readPtr = 0;   //Pointer where we are starting with reading
int canFifo_writePtr = 0;  //Pointer where we are starting with writing
bool flagOverflow = false; //Overflow flag
struct CanMessage canMessageFifo[CAN_BUFFER_ITEMS]; //FIFO buffer with received CAN messages for user
uint32_t canBaudrate;

/* Private methods -----------------------------------------------------------*/
static ErrorCodes Can_ResetFifo(void);

/**
* @brief  Return amount of CAN messages in a buffer
* @retval How many messages is in CAN buffer ready for receving
*/
ErrorCodes Can_Rx_GetCount(uint32_t* count)
{
	//Check if buffer has overflown
	if (flagOverflow == true)
	{
		Can_ResetFifo();
		return ERROR_DATA_OVERFLOW;
	}

	//this setup is being used as a ring buffer
	if(canFifo_readPtr > canFifo_writePtr)
	{
		//If read data has higher index than write data
		//this means that write data index has jumped on start
		*count = CAN_BUFFER_ITEMS - canFifo_readPtr + canFifo_writePtr;
	}
	else
	{
		*count = canFifo_writePtr - canFifo_readPtr;
	}
	return ERROR_OK;
}

/**
* @brief  Receive one CAN message from buffer
* @param  msg: Structure where message is going to be copied
* @retval ERROR_OK: Received data are written in provided variables
*         ERROR_DATA_EMPTY: In buffer are no data available
*         ERROR_DATA_OVERFLOW: No data in buffer because pointers were reset
*/
ErrorCodes Can_Rx(CanMessage *msg)
{
	CanMessage canMsg;
	int i;
	//If buffer has overflow, reset pointers in FIFO
	if (flagOverflow == true)
	{
		Can_ResetFifo();
		return ERROR_DATA_OVERFLOW;
	}

	//If pointers are same before reading and without buffer overflow flag, then buffer is empty
	if (canFifo_writePtr == canFifo_readPtr)
	{
		return ERROR_DATA_EMPTY;
	}

	//Read next message from buffer and copy it
	canMsg = canMessageFifo[canFifo_readPtr];
	msg->Dlc = canMsg.Dlc;
	msg->Id = canMsg.Id;
	msg->Timestamp = canMsg.Timestamp;
	for (i = 0; i < canMsg.Dlc; i++)
	{
		msg->Frame[i] = canMsg.Frame[i];
	}

	//Move read pointer, if we are on the end of ring buffer, reset pointer
	canFifo_readPtr++;
	if (canFifo_readPtr >= CAN_BUFFER_ITEMS)
	{
		canFifo_readPtr = 0;
	}

	return ERROR_OK;
}

ErrorCodes Can_ResetFifo()
{
	canFifo_readPtr = 0;
	canFifo_writePtr = 0;
	flagOverflow = false;
	return ERROR_OK;
}
/**
* @brief  Enable CAN peripheral
* @param  baudrate: bits per seconds which we want to set CAN peripheral on. Max 1MBit/s
* @param  mode: We can set device into passive or active mode
* @retval ERROR_OK: Setup OK
*         ERROR_GENERAL: Name of CAN peripheral which we want to work with is not supported on this device
*/
ErrorCodes Can_Enable(uint32_t baudrate, CanMode mode)
{   
	CAN_FilterTypeDef  sFilterConfig;
	GPIO_InitTypeDef   GPIO_InitStruct;
    // Enable GPIO clock 
    //RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

    // Connect CAN pins 
    //GPIO_PinAFConfig(GPIOB, GPIO_PinSource5, GPIO_AF_CAN2);
    //GPIO_PinAFConfig(GPIOB, GPIO_PinSource13, GPIO_AF_CAN2); 
    
    // Configure CAN RX and TX pins 
    /*GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_13;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
    GPIO_Init(GPIOB, &GPIO_InitStructure);*/

    // CAN configuration ********************************************************/  
  
    // Enable CAN clock - Necessary to enable CAN1 | CAN2, otherwise 1FFFFFB DLC=0 will be sent
    //RCC_APB1PeriphClockCmd((RCC_APB1Periph_CAN2 | RCC_APB1Periph_CAN1), ENABLE);

    // CAN register init 
    //CAN_DeInit(CAN2);
	
	/*##-1- Enable peripherals and GPIO Clocks #################################*/
	/* CAN1 Periph clock enable */
  	__HAL_RCC_CAN1_CLK_ENABLE();
	__HAL_RCC_CAN2_CLK_ENABLE();

  	/* Enable GPIO clock ****************************************/
  	__HAL_RCC_GPIOB_CLK_ENABLE();

  	/*##-2- Configure peripheral GPIO ##########################################*/

  	/* CAN1 RX GPIO pin configuration */
  	GPIO_InitStruct.Pin = GPIO_PIN_5;
  	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  	GPIO_InitStruct.Pull = GPIO_PULLUP;
  	GPIO_InitStruct.Alternate =  GPIO_AF9_CAN2;

  	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  	/*##-3- Configure the NVIC #################################################*/
  	/* NVIC configuration for CAN1 Reception complete interrupt */
  	HAL_NVIC_SetPriority(CAN2_RX0_IRQn, 1, 0);
  	HAL_NVIC_EnableIRQ(CAN2_RX0_IRQn);

    // CAN cell init 
	hcan.Instance = CAN2;
	hcan.Init.TimeTriggeredMode = DISABLE;
	hcan.Init.AutoBusOff = DISABLE;
	hcan.Init.AutoWakeUp = DISABLE;
	hcan.Init.AutoRetransmission = DISABLE;
	hcan.Init.ReceiveFifoLocked = DISABLE;
	hcan.Init.TransmitFifoPriority = DISABLE;
    if(mode == CAN_ACTIVE)
    {
        hcan.Init.Mode = CAN_MODE_NORMAL;
    }
    else if(mode == CAN_PASSIVE)
    {
        hcan.Init.Mode = CAN_MODE_SILENT;
    }
    else
    {
        return ERROR_GENERAL;
    }

    /* 
     * Using: Baudrate = frequency / (prescaler * (sjw + bs1 + bs2)));
     * 
     * Baudrate  Psc    SJW  BS1  BS2      APB1 Freq
     * 100000    300    1    8    5        420000000    CAN_BITRATE_10K
     * 200000    150    1    8    5        420000000    CAN_BITRATE_20K
     * 500000    60     1    8    5        420000000    CAN_BITRATE_50K
     * 1000000   30     1    8    5        420000000    CAN_BITRATE_100K
     * 1250000   24     1    8    5        420000000    CAN_BITRATE_125K
     * 2500000   12     1    8    5        420000000    CAN_BITRATE_250K
     * 5000000   6      1    8    5        420000000    CAN_BITRATE_500K
     * 7500000   4      1    8    5        420000000    CAN_BITRATE_750K
     * 10000000  3      1    8    5        420000000    CAN_BITRATE_1000K
     */

	//RCC_GetClocksFreq(&RCC_Clocks);
    //if(RCC_Clocks.PCLK1_Frequency == 42000000)
	if(true)
    {
        //CAN_InitStructure.CAN_SJW = CAN_SJW_1tq;
        //CAN_InitStructure.CAN_BS1 = CAN_BS1_8tq;
        //CAN_InitStructure.CAN_BS2 = CAN_BS2_5tq;  
		hcan.Init.SyncJumpWidth = CAN_SJW_1TQ;
		hcan.Init.TimeSeg1 = CAN_BS1_4TQ;
		hcan.Init.TimeSeg2 = CAN_BS2_3TQ;
        switch(baudrate)
        {
            case 10000:
                hcan.Init.Prescaler = 300;
                break;
  	          case 20000:
                hcan.Init.Prescaler = 150;
                break;
            case 50000:
                hcan.Init.Prescaler = 60;
                break;
            case 100000:
                hcan.Init.Prescaler = 30;
                break;
            case 125000:
                hcan.Init.Prescaler = 24;
                break;
            case 250000:
                hcan.Init.Prescaler = 12;
                break;
            case 500000:
                hcan.Init.Prescaler = 6;
                break;
            case 750000:
                hcan.Init.Prescaler = 4;
                break;
            case 1000000:
                hcan.Init.Prescaler = 3;
                break;
            default:
                return ERROR_CAN_UNS_BAUDRATE;
        }
		canBaudrate = baudrate;
    }
    else
    {
        return ERROR_CAN_INV_CLK;
    }
    HAL_CAN_Init(&hcan);


    // CAN filter init into transparent mode
    /*##-2- Configure the CAN Filter ###########################################*/
	sFilterConfig.FilterBank = 0;
  	sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
  	sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
  	sFilterConfig.FilterIdHigh = 0x0000;
  	sFilterConfig.FilterIdLow = 0x0000;
  	sFilterConfig.FilterMaskIdHigh = 0x0000;
  	sFilterConfig.FilterMaskIdLow = 0x0000;
  	sFilterConfig.FilterFIFOAssignment = CAN_RX_FIFO0;
  	sFilterConfig.FilterActivation = ENABLE;
  	sFilterConfig.SlaveStartFilterBank = 14;
	if (HAL_CAN_ConfigFilter(&hcan, &sFilterConfig) != HAL_OK)
	{
		/* Filter configuration Error */
    	return ERROR_GENERAL;
  	}

	/*##-3- Start the CAN peripheral ###########################################*/
  	if (HAL_CAN_Start(&hcan) != HAL_OK)
  	{
    	/* Start Error */
    	return ERROR_GENERAL;
  	}

	/*##-4- Activate CAN RX notification #######################################*/
  	if (HAL_CAN_ActivateNotification(&hcan, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK)
  	{
    	/* Notification Error */
    	return ERROR_GENERAL;
  	}
  
    // Enable FIFO 0 message pending Interrupt 
    /*CAN_ITConfig(CAN2, CAN_IT_FMP0, ENABLE);
  
    // Enable the CAN2 global Interrupt 
    NVIC_InitStructure.NVIC_IRQChannel = CAN2_RX0_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);*/
    return ERROR_OK;
}

/**
* @brief  Disable CAN peripheral
*/
ErrorCodes Can_Disable()
{
    //CAN_DeInit(CAN2);
    //Can_ResetFifo();
    return ERROR_OK;
}

void CAN2_RX0_IRQHandler (void)
{
	CAN_RxHeaderTypeDef RxHeader;
	uint8_t RxData[8];
	int i;
	uint32_t canId;
	
	/* Get RX message */
  	if (HAL_CAN_GetRxMessage(&hcan, CAN_RX_FIFO0, &RxHeader, RxData) != HAL_OK)
  	{
    	/* Reception Error */
    	//Error_Handler();
			goto CanIsrEnd;
  	}
	
	//Save message into statistics
	if(RxHeader.IDE == CAN_ID_EXT)
	{
		Stats_CanMessage_RxAdd(RxHeader.DLC, 1, canBaudrate);
	}
	else
	{
		Stats_CanMessage_RxAdd(RxHeader.DLC, 0, canBaudrate);
	}

	//Save received CAN message to FIFO buffer
	//Ignore any new message when buffer has overflown
	if (flagOverflow == true)
	{
		goto CanIsrEnd;
	}
	
	//Get CAN ID
	if(RxHeader.IDE == CAN_ID_STD)
	{
		canId = RxHeader.StdId;
	}
	else
	{
		canId = RxHeader.ExtId;
	}

	//Add message to the buffer
	canMessageFifo[canFifo_writePtr].Dlc = RxHeader.DLC;
	canMessageFifo[canFifo_writePtr].Id = canId;
	canMessageFifo[canFifo_writePtr].Timestamp = GetTime_ms();
	
	for (i = 0; i < RxHeader.DLC; i++)
	{
		canMessageFifo[canFifo_writePtr].Frame[i] = RxData[i];
	}

	//Move write pointer, if we are on the end of ring buffer, reset pointer
	canFifo_writePtr++;
	if (canFifo_writePtr >= CAN_BUFFER_ITEMS)
	{
		canFifo_writePtr = 0;
	}
	//If pointer are same after writing, then we made a buffer overflow of RX FIFO
	if (canFifo_writePtr == canFifo_readPtr)
	{
		flagOverflow = true;
	}

CanIsrEnd:
	return;	
}
