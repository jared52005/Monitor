/*******************************************************************************
* @file    CanIf.h
* @version V1.2
* @date    13-November-2018
* @brief   Implementation of basic operations with CAN peripheral
******************************************************************************
* @attention
******************************************************************************  
*/ 
#include <stdint.h>
#include <stdbool.h>
#include "CanIf.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_can.h"

/* Private definitions -------------------------------------------------------*/
#if defined(OLIMEXP405_Cv10) || defined(STM32F40GEVAL)
#define BOARD_CAN_IF CAN1
#endif

/* Private variables ---------------------------------------------------------*/
int canFifo0_readPtr = 0;   //Pointer where we are starting with reading
int canFifo0_writePtr = 0;  //Pointer where we are starting with writing
bool flagOverflow0 = false; //Overflow flag
struct CanMessage canMessageFifo0[CAN_BUFFER_ITEMS]; //FIFO buffer with received CAN messages for user

ErrorCodes canLastError;

can_bitrate _canBaudrate = CAN_BITRATE_500K;
CanMode _canMode = CAN_ACTIVE;

/* Private methods -----------------------------------------------------------*/
static ErrorCodes Can_ResetFifo_0(void);
static void Can_WaitReady (CAN_TypeDef* CANx);

/**
* @brief  Receive one CAN message from buffer
* @param  canMsg: Strucutre, where received CAN message will be written
* @retval ERROR_OK: Received data are written in provided variables
*         CAN_ERROR_DATA_EMPTY: In buffer are no data available
*         CAN_ERROR_DATA_OVERFLOW: No data in buffer because pointers were reset
*/
ErrorCodes Can_Rx(CanMessage *canMsg)
{
    //If buffer has overflow, reset pointers in FIFO
    if (flagOverflow0 == true)
    {
        Can_ResetFifo_0();
        return ERROR_DATA_OVERFLOW;
    }

    //If pointers are same before reading and without buffer overflow flag, then buffer is empty
    if (canFifo0_writePtr == canFifo0_readPtr)
    {
        return ERROR_DATA_EMPTY;
    }

    //Read next message from buffer and copy it
    *canMsg = canMessageFifo0[canFifo0_readPtr];

    //Move read pointer, if we are on the end of ring buffer, reset pointer
    canFifo0_readPtr++;
    if (canFifo0_readPtr >= CAN_BUFFER_ITEMS)
    {
        canFifo0_readPtr = 0;
    }

    return ERROR_OK;
}

ErrorCodes Can_ResetFifo_0()
{
    canFifo0_readPtr = 0;
    canFifo0_writePtr = 0;
    flagOverflow0 = false;
    return ERROR_OK;
}

ErrorCodes Can_Tx_State(void)
{
    ErrorCodes status;
    uint8_t txStatus;
    txStatus = CAN_TransmitStatus(BOARD_CAN_IF, 0);
    switch(txStatus)
    {
        case CAN_TxStatus_Pending:
            status = ERROR_CAN_TX_PENDING;
          break;
        case CAN_TxStatus_Ok:
            status = ERROR_OK;
          break;
        case CAN_TxStatus_Failed:
            status = ERROR_GENERAL;
        break;
        default:
            status = ERROR_GENERAL;
    }
    return status;
}

/**
* @brief  Cancel transmission of given CAN device
* @retval ERROR_OK = Transmit of CAN message was cancelled
*/
ErrorCodes Can_Tx_Cancel(void)
{
    CAN_CancelTransmit(CAN1, 0);
    return ERROR_OK;
}

/**
* @brief  Transmit message onto CAN bus
* @param  name: Name of CAN device which we want to use for communication
* @param  id: Identification which we want to use for TX
* @retval ERROR_OK: Data were transmitted on CAN
*         CAN_ERROR_TOO_BIG: We are trying to transmit more than 8 bytes of data
*         CAN_ERROR_NOT_ENABLED: CAN peripheral is not enabled
*         CAN_ERROR_IS_PASSIVE: Can't transmit on CAN when in passive mode
*/
ErrorCodes Can_Tx(CanMessage* txMsg)
{
    uint8_t i;
    CanTxMsg TxMessage;
     
    if(txMsg->Dlc > 8)
    {
        return ERROR_CAN_TOO_BIG;
    }
    
    if(txMsg->Id<0x800)
    {
        TxMessage.StdId = txMsg->Id;
        TxMessage.IDE = CAN_ID_STD;
    }
    else
    {
        TxMessage.ExtId = txMsg->Id;
        TxMessage.IDE = CAN_ID_EXT;
    }
    TxMessage.RTR = CAN_RTR_DATA;  //This is a data frame
    TxMessage.DLC = txMsg->Dlc;
  
    //Copy data into CAN frame
    for(i = 0; i < txMsg->Dlc; i++)
    {    
        TxMessage.Data[i] = txMsg->Frame[i];
    }
    
    //Wait until Transmit Message box 0 is ready
    Can_WaitReady(BOARD_CAN_IF);
    canLastError = ERROR_OK; //Remove last error from CAN
    //Transmit message only on mailbox 0
    if(CAN_Transmit_Mailbox0(BOARD_CAN_IF,&TxMessage) == CAN_TxStatus_NoMailBox )
    {
        //Error during transmission
        return ERROR_GENERAL;
    }
    else
    {
        //CAN message sent or prepared for sending
        return ERROR_OK;
    }
}

ErrorCodes Can_Tx_LastError(void)
{
    return canLastError;
}

//Wait until Mailbox 0 is empty
static void Can_WaitReady (CAN_TypeDef* CANx)  
{
  while ((CANx->TSR & CAN_TSR_TME0) == 0);  /* Transmit mailbox 0 is empty    */
}

ErrorCodes Can_Baudrate_Set(can_bitrate baudrate)
{
    _canBaudrate = baudrate;
    return ERROR_OK;
}

ErrorCodes Can_Mode_Set(CanMode mode)
{
    _canMode = mode;
    return ERROR_OK;
}

#if defined(STM32F40GEVAL)
/*
* @brief  Enable CAN peripheral
* @retval ERROR_OK: Setup OK
*/
ErrorCodes Can_Enable(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    RCC_ClocksTypeDef RCC_Clocks;
	  CAN_InitTypeDef        CAN_InitStructure;
	  CAN_FilterInitTypeDef  CAN_FilterInitStructure;
    
    // Enable GPIO clock 
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);

    // Connect CAN pins 
    GPIO_PinAFConfig(GPIOD, GPIO_PinSource1, GPIO_AF_CAN1);
    GPIO_PinAFConfig(GPIOD, GPIO_PinSource0, GPIO_AF_CAN1); 
    
    // Configure CAN RX and TX pins 
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
    GPIO_Init(GPIOD, &GPIO_InitStructure);

    // CAN configuration ********************************************************/  
  
    // Enable CAN clock 
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1, ENABLE);

    // CAN register init 
    CAN_DeInit(CAN1);
    Can_ResetFifo_0();
    //Load default data from CAN HAL driver
    CAN_StructInit(&CAN_InitStructure);

    // CAN cell init 
    CAN_InitStructure.CAN_TTCM = DISABLE;
    CAN_InitStructure.CAN_ABOM = DISABLE;
    CAN_InitStructure.CAN_AWUM = DISABLE;
    CAN_InitStructure.CAN_NART = DISABLE;
    CAN_InitStructure.CAN_RFLM = DISABLE;
    CAN_InitStructure.CAN_TXFP = DISABLE;
    if(_canMode == CAN_ACTIVE)
    {
        CAN_InitStructure.CAN_Mode = CAN_Mode_Normal;
    }
    else if(_canMode == CAN_PASSIVE)
    {
        CAN_InitStructure.CAN_Mode = CAN_Mode_Silent;
    }
    else if(_canMode == CAN_LOOPBACK)
    {
        CAN_InitStructure.CAN_Mode = CAN_Mode_LoopBack;
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
		
    //By default expected 8MHz XTAL
    RCC_GetClocksFreq(&RCC_Clocks, (uint32_t)HSE_VALUE);
    if(RCC_Clocks.PCLK1_Frequency == 42000000)
    {
        CAN_InitStructure.CAN_SJW = CAN_SJW_1tq;
        CAN_InitStructure.CAN_BS1 = CAN_BS1_8tq;
        CAN_InitStructure.CAN_BS2 = CAN_BS2_5tq;  
        switch(_canBaudrate)
        {
            case CAN_BITRATE_10K:
                CAN_InitStructure.CAN_Prescaler = 300;
                break;
            case CAN_BITRATE_20K:
                CAN_InitStructure.CAN_Prescaler = 150;
                break;
            case CAN_BITRATE_50K:
                CAN_InitStructure.CAN_Prescaler = 60;
                break;
            case CAN_BITRATE_100K:
                CAN_InitStructure.CAN_Prescaler = 30;
                break;
            case CAN_BITRATE_125K:
                CAN_InitStructure.CAN_Prescaler = 24;
                break;
            case CAN_BITRATE_250K:
                CAN_InitStructure.CAN_Prescaler = 12;
                break;
            case CAN_BITRATE_500K:
                CAN_InitStructure.CAN_Prescaler = 6;
                break;
            case CAN_BITRATE_750K:
                CAN_InitStructure.CAN_Prescaler = 4;
                break;
            case CAN_BITRATE_1000K:
                CAN_InitStructure.CAN_Prescaler = 3;
                break;
            default:
                return ERROR_GENERAL;
        }
    }
    else
    {
        return ERROR_GENERAL;
    }
    CAN_Init(CAN1, &CAN_InitStructure);

    // CAN filter init into transparent mode
    CAN_FilterInitStructure.CAN_FilterNumber = 0;
    CAN_FilterInitStructure.CAN_FilterMode = CAN_FilterMode_IdMask;
    CAN_FilterInitStructure.CAN_FilterScale = CAN_FilterScale_32bit;
    CAN_FilterInitStructure.CAN_FilterIdHigh = 0;
    CAN_FilterInitStructure.CAN_FilterIdLow = 0;
    CAN_FilterInitStructure.CAN_FilterMaskIdHigh = 0;
    CAN_FilterInitStructure.CAN_FilterMaskIdLow = 0;
    CAN_FilterInitStructure.CAN_FilterFIFOAssignment = 0;
    CAN_FilterInitStructure.CAN_FilterActivation = ENABLE;
    CAN_FilterInit(CAN1, &CAN_FilterInitStructure);
  
    // Enable FIFO 0 message pending Interrupt 
    CAN_ITConfig(CAN1, CAN_IT_FMP0, ENABLE);
  
    // Enable the CAN1 gloabal Interrupt 
    NVIC_InitStructure.NVIC_IRQChannel = CAN1_RX0_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 13;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    CAN_ITConfig(CAN1, CAN_IT_LEC | CAN_IT_ERR, ENABLE);
    NVIC_InitStructure.NVIC_IRQChannel = CAN1_SCE_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 13;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    return ERROR_OK;
}
#endif

#if defined(OLIMEXP405_Cv10)
/**
* @brief  Enable CAN peripheral
* @retval ERROR_OK: Setup OK
*/
ErrorCodes Can_Enable(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    RCC_ClocksTypeDef RCC_Clocks;
	CAN_InitTypeDef        CAN_InitStructure;
	CAN_FilterInitTypeDef  CAN_FilterInitStructure;
    
    // Enable GPIO clock 
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

    // Connect CAN pins PB8 and PB9
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource8, GPIO_AF_CAN1);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource9, GPIO_AF_CAN1); 
    
    // Configure CAN RX and TX pins 
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    // CAN configuration ********************************************************/  
  
    // Enable CAN clock 
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1, ENABLE);

    // CAN register init 
    CAN_DeInit(CAN1);
    Can_ResetFifo_0();
    //Load default data from CAN HAL driver
    CAN_StructInit(&CAN_InitStructure);

    // CAN cell init 
    CAN_InitStructure.CAN_TTCM = DISABLE;
    CAN_InitStructure.CAN_ABOM = DISABLE;
    CAN_InitStructure.CAN_AWUM = DISABLE;
    CAN_InitStructure.CAN_NART = DISABLE;
    CAN_InitStructure.CAN_RFLM = DISABLE;
    CAN_InitStructure.CAN_TXFP = DISABLE;
    if(_canMode == CAN_ACTIVE)
    {
        CAN_InitStructure.CAN_Mode = CAN_Mode_Normal;
    }
    else if(_canMode == CAN_PASSIVE)
    {
        CAN_InitStructure.CAN_Mode = CAN_Mode_Silent;
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
		
    //By default expected 8MHz XTAL
    RCC_GetClocksFreq(&RCC_Clocks, (uint32_t)HSE_VALUE);
    if(RCC_Clocks.PCLK1_Frequency == 42000000)
    {
        CAN_InitStructure.CAN_SJW = CAN_SJW_1tq;
        CAN_InitStructure.CAN_BS1 = CAN_BS1_8tq;
        CAN_InitStructure.CAN_BS2 = CAN_BS2_5tq;  
        switch(_canBaudrate)
        {
            case CAN_BITRATE_10K:
                CAN_InitStructure.CAN_Prescaler = 300;
                break;
            case CAN_BITRATE_20K:
                CAN_InitStructure.CAN_Prescaler = 150;
                break;
            case CAN_BITRATE_50K:
                CAN_InitStructure.CAN_Prescaler = 60;
                break;
            case CAN_BITRATE_100K:
                CAN_InitStructure.CAN_Prescaler = 30;
                break;
            case CAN_BITRATE_125K:
                CAN_InitStructure.CAN_Prescaler = 24;
                break;
            case CAN_BITRATE_250K:
                CAN_InitStructure.CAN_Prescaler = 12;
                break;
            case CAN_BITRATE_500K:
                CAN_InitStructure.CAN_Prescaler = 6;
                break;
            case CAN_BITRATE_750K:
                CAN_InitStructure.CAN_Prescaler = 4;
                break;
            case CAN_BITRATE_1000K:
                CAN_InitStructure.CAN_Prescaler = 3;
                break;
            default:
                return ERROR_GENERAL;
        }
    }
    else
    {
        return ERROR_GENERAL;
    }
    CAN_Init(CAN1, &CAN_InitStructure);

    // CAN filter init into transparent mode
    CAN_FilterInitStructure.CAN_FilterNumber = 0;
    CAN_FilterInitStructure.CAN_FilterMode = CAN_FilterMode_IdMask;
    CAN_FilterInitStructure.CAN_FilterScale = CAN_FilterScale_32bit;
    CAN_FilterInitStructure.CAN_FilterIdHigh = 0;
    CAN_FilterInitStructure.CAN_FilterIdLow = 0;
    CAN_FilterInitStructure.CAN_FilterMaskIdHigh = 0;
    CAN_FilterInitStructure.CAN_FilterMaskIdLow = 0;
    CAN_FilterInitStructure.CAN_FilterFIFOAssignment = 0;
    CAN_FilterInitStructure.CAN_FilterActivation = ENABLE;
    CAN_FilterInit(CAN1, &CAN_FilterInitStructure);
  
    // Enable FIFO 0 message pending Interrupt 
    CAN_ITConfig(CAN1, CAN_IT_FMP0, ENABLE);
  
    // Enable the CAN1 gloabal Interrupt 
    NVIC_InitStructure.NVIC_IRQChannel = CAN1_RX0_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 13;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    CAN_ITConfig(CAN1, CAN_IT_LEC | CAN_IT_ERR, ENABLE);
    NVIC_InitStructure.NVIC_IRQChannel = CAN1_SCE_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 13;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    return ERROR_OK;
}
#endif

void CAN1_RX0_IRQHandler (void)
{
    CanRxMsg RxMessage;
    int i;
    uint32_t canId;
    CanIdType cIdType;
    
    //Check if message is pending for FIFO0
    if(CAN_GetITStatus(BOARD_CAN_IF, CAN_IT_FMP0) == RESET) 
    {
        //Not for us
        return;
    }

    CAN_Receive(BOARD_CAN_IF,CAN_FIFO0, &RxMessage);
    
    //Save received CAN message to FIFO buffer
    //Ignore any new message when buffer has overflown
    if (flagOverflow0 == true)
    {
        goto CanIsrEnd;
    }
    
    //Get CAN ID
    if(RxMessage.IDE == CAN_Id_Standard)
    {
        canId = RxMessage.StdId;
        cIdType = CAN_ID_TYPE_STD;
    }
    else
    {
        canId = RxMessage.ExtId;
        cIdType = CAN_ID_TYPE_EXT;
    }
    
    //Add message to the buffer
    canMessageFifo0[canFifo0_writePtr].Dlc = RxMessage.DLC;
    canMessageFifo0[canFifo0_writePtr].Id = canId;
    canMessageFifo0[canFifo0_writePtr].ID_Type = cIdType;
    for (i = 0; i < RxMessage.DLC; i++)
    {
        canMessageFifo0[canFifo0_writePtr].Frame[i] = RxMessage.Data[i];
    }
    //Move write pointer, if we are on the end of ring buffer, reset pointer
    canFifo0_writePtr++;
    if (canFifo0_writePtr >= CAN_BUFFER_ITEMS)
    {
        canFifo0_writePtr = 0;
    }
    //If pointer are same after writing, then we made a buffer overflow of RX FIFO
    if (canFifo0_writePtr == canFifo0_readPtr)
    {
        flagOverflow0 = true;
    }
    
CanIsrEnd:
    CAN_ClearITPendingBit(BOARD_CAN_IF,CAN_IT_FMP0);
    return;    
}

void CAN1_SCE_IRQHandler( void )
 {
    if (CAN_GetITStatus( BOARD_CAN_IF, CAN_IT_LEC ) == SET )
    {
        canLastError = CAN_GetLastErrorCode(BOARD_CAN_IF);
        canLastError >>=4; //LEC is only masked from CAN_ESR register, move it down by 4 bits;
        CAN_ClearITPendingBit(BOARD_CAN_IF, CAN_IT_LEC );
        //At this moment we can get into endless loop of errors. Cancel last transmitted message
        Can_Tx_Cancel();
    }
}
