/*******************************************************************************
* @file    UartIf.c
* @version V1.0
* @date    30-April-2020
* @brief   Drivers for communication with STM32F4xx UART devices
******************************************************************************
* @attention
******************************************************************************  
*/ 

#include "UartIf.h"
#include "stm32f4xx_usart.h"
#include "stm32f4xx_rcc.h"

/* Private definitions -------------------------------------------------------*/
#if defined(STM32F40GEVAL) || defined(BOARD_STM32F405_B) || defined(BOARD_STM32F405_C)
#define BOARD_UART_IF USART3
#endif

//Olimex P405 talking via RS232 + USART2
#ifdef OLIMEXP405_Cv10
#define BOARD_UART_IF USART2
#endif

/* Private variables ---------------------------------------------------------*/
static uint8_t uartBuffer[255];    //Ring buffer on data comming from UART
static uint8_t uartBufferReadPtr;  //Pointer from which we are reading data in Ring Buffer
static uint8_t uartBufferWritePtr; //Pointer on which we are writing data in Ring Buffer
static uint8_t uartBufferOverrun;  //Not equal zero, if buffer was overrun

/* Private function prototypes -----------------------------------------------*/

/**
* @brief  Write one character into KLINE UART interface
* @param  c: Read character
* @retval UART_OK
*/
ErrorCodes Uart_Putc(volatile char c)
{
    //Wait until transmit data register is empty
    while (!USART_GetFlagStatus(BOARD_UART_IF, USART_FLAG_TXE));
    //Send a char using BOARD_UART_IF (USART3, USART1, ...)
    USART_SendData(BOARD_UART_IF, c);
    return ERROR_OK;
}

/**
* @brief  Puts character to debugging UART
* @param  c: character to send over UART
* @retval UART_OK
*/
void Uart_Putc_Debug(volatile char c)
{
    if ((ITM->TCR & ITM_TCR_ITMENA_Msk) &&  // ITM enabled 
          (ITM->TER & (1UL << 0))           // ITM Port #0 enabled
        ) 
    {
        while (ITM->PORT[0].u32 == 0);   // Wait for available 
        ITM->PORT[0].u8 = (uint8_t)c;    // Send character 
    }
}

/**
* @brief  Gets received character from internal buffer
* @param  c: pointer to store new character to
* @retval Character status:
*            - UART_DATA_OK: Character is valid inside *c_str
*            - UART_DATA_EMPTY: No character in *c
*            - UART_RECEIVE_BUFFER_OVER: Ring buffer overflow and got reset
*/
ErrorCodes Uart_Getc(uint8_t* c)
{
    //Check if we did overrun
    if(uartBufferOverrun != 0)
    {
        //Inform only once
        uartBufferOverrun = 0;
        return ERROR_DATA_OVERFLOW;
    }
    if(uartBufferReadPtr == uartBufferWritePtr)
    {
        return ERROR_DATA_EMPTY;
    }
    else
    {
        *c = uartBuffer[uartBufferReadPtr];
        uartBufferReadPtr++;
        return ERROR_OK;
    }
}

/**
* @brief  Sends array of data to UART
* @param  DataArray: Pointer to 8-bit data array to be sent over UART
* @param  Length: Number of elements to sent in units of bytes
* @param  name: Target UART which we want to use
* @retval Sending status
*/
ErrorCodes Uart_Tx(uint8_t* DataArray, uint32_t Length)
{
    uint32_t i;
    ErrorCodes result = ERROR_GENERAL;
    //Send data in generic way = write byte-per-byte into interface until finished
    for(i = 0; i<Length; i++)
    {
        result = Uart_Putc(DataArray[i]);
        //Check if byte was written OK
        if(result != UART_OK)
        {
            //Nope, break the loop
            break;
        }
    }
    return result;
}

/**
* @brief  Receive array of data from UART
* @param  DataArray: Write data into this buffer
* @param  MaxLength: Size of buffer
* @retval Amount of received bytes
*/
uint32_t Uart_Rx(uint8_t* DataArray, uint32_t MaxLength)
{
    uint32_t i;
    ErrorCodes result = ERROR_GENERAL;
    //Read from UART buffer as long string as possible
    for(i = 0; i<MaxLength; i++)
    {
        result = Uart_Getc(&DataArray[i]);
        if(result != UART_DATA_OK)
        {
            //End of received data
            break;
        }
    }
    return i;
}

/**
* @brief  Disable UART
* @retval Sending status
*/
ErrorCodes Uart_Disable(void)
{
    //USART_Cmd(BOARD_UART_IF, DISABLE);
    return ERROR_OK;
}

//USART2 (RS232 connector) Olimex P405
#ifdef OLIMEXP405_Cv10
ErrorCodes Uart_Enable(uint32_t baudrate)
{
    USART_InitTypeDef USART_InitStruct;
    GPIO_InitTypeDef GPIO_InitStruct;
    NVIC_InitTypeDef NVIC_InitStruct;

    // Enable clock for GPIOA
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
    // Enable clock for USART2
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
    // Connect PA2 to USART2_Tx
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_USART2);
    // Connect PA3 to USART2_Rx
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource3, GPIO_AF_USART2);
    
    // Initialization of GPIOA
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOA, &GPIO_InitStruct);

    // Initialization of USART2
    USART_InitStruct.USART_BaudRate = baudrate;
    USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStruct.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_InitStruct.USART_Parity = USART_Parity_No;
    USART_InitStruct.USART_StopBits = USART_StopBits_1;
    USART_InitStruct.USART_WordLength = USART_WordLength_8b;
    USART_Init(USART2, &USART_InitStruct, HSE_VALUE);
    
    // Enable USART2
    USART_Cmd(USART2, ENABLE);
        
    // Enable RX interrupt
    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
    /**
    * Set Channel to USART2
    * Set Channel Cmd to enable. That will enable USART2 channel in NVIC
    * Set Both priorities to 0. This means high priority
    *
    * Initialize NVIC
    */
    NVIC_InitStruct.NVIC_IRQChannel = USART2_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0;
    NVIC_Init(&NVIC_InitStruct);
    return ERROR_OK;
}

/**
* @brief  IRQ entrypoint for data from UART3 (KLINE)
*/
void USART2_IRQHandler(void)
{
    uint8_t c;
    //Check if we have received data in UART1 peripheral
    //And read them all into buffer
    while(USART_GetITStatus(USART2, USART_IT_RXNE) == SET)
    {
        //Read data from UART1 RX register
        c = (uint8_t)USART_ReceiveData(USART2);
        //Write them into ring buffer on move write pointer +1
        uartBuffer[uartBufferWritePtr] = c;
        uartBufferWritePtr++; //pointers are 8bit long, so they will be always in range
        //Check if we overrun
        if(uartBufferWritePtr == uartBufferReadPtr)
        {
            //yep, we did, reset pointers
            uartBufferOverrun = 1;
            uartBufferWritePtr = 0;
            uartBufferReadPtr = 0;
        }
    }
}

#endif

#if defined(BOARD_STM32F405_B) || defined(BOARD_STM32F405_C)
ErrorCodes Uart_Enable(uint32_t baudrate)
{
    USART_InitTypeDef USART_InitStruct;
    GPIO_InitTypeDef GPIO_InitStruct;
    NVIC_InitTypeDef NVIC_InitStruct;

    //Setup buffer pointers
    uartBufferOverrun = 0;
    uartBufferWritePtr = 0;
    uartBufferReadPtr = 0;
    
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource10, GPIO_AF_USART3);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource11, GPIO_AF_USART3);
    
    // Initialization of GPIOA
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOB, &GPIO_InitStruct);

    // Initialization of USART1
    USART_InitStruct.USART_BaudRate = baudrate;
    USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStruct.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_InitStruct.USART_Parity = USART_Parity_No;
    USART_InitStruct.USART_StopBits = USART_StopBits_1;
    USART_InitStruct.USART_WordLength = USART_WordLength_8b;
    USART_Init(USART3, &USART_InitStruct, (uint32_t)HSE_VALUE);
    
    // Enable USART1
    USART_Cmd(USART3, ENABLE);
    
    // Enable RX interrupt
    USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);
    /**
    * Set Channel to USART1
    * Set Channel Cmd to enable. That will enable USART1 channel in NVIC
    * Set Both priorities to 0. This means high priority
    *
    * Initialize NVIC
    */
    NVIC_InitStruct.NVIC_IRQChannel = USART3_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0;
    NVIC_Init(&NVIC_InitStruct);
    return UART_OK;
}

/**
* @brief  IRQ entrypoint for data from UART1
*/
void USART3_IRQHandler(void)
{
    uint8_t c;
    //Check if we have received data in UART1 peripheral
    //And read them all into buffer
    while(USART_GetITStatus(USART3, USART_IT_RXNE) == SET)
  {
        //Read data from UART1 RX register
        c = (uint8_t)USART_ReceiveData(USART3);
        //Write them into ring buffer on move write pointer +1
        uartBuffer[uartBufferWritePtr] = c;
        uartBufferWritePtr++; //pointers are 8bit long, so they will be always in range
        //Check if we overrun
        if(uartBufferWritePtr == uartBufferReadPtr)
        {
            //yep, we did, reset pointers
            uartBufferOverrun = 1;
            uartBufferWritePtr = 0;
            uartBufferReadPtr = 0;
        }
    }
}
#endif

//STM32F40G-Eval board RS232 USART3 on PC10/PC11
#ifdef STM32F40GEVAL
ErrorCodes Uart_Enable(uint32_t baudrate)
{
    USART_InitTypeDef USART_InitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;
    NVIC_InitTypeDef NVIC_InitStruct;
    
    //Setup buffer pointers
    uartBufferOverrun = 0;
    uartBufferWritePtr = 0;
    uartBufferReadPtr = 0;

    /* --------------------------- System Clocks Configuration -----------------*/
    /* USART3 clock enable */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
    /* GPIOC clock enable */
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);

    /*-------------------------- GPIO Configuration ----------------------------*/
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
    
    /* Connect USART pins to AF */
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource10, GPIO_AF_USART3);
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource11, GPIO_AF_USART3);
    
    /* USARTx configuration ------------------------------------------------------*/
    USART_InitStructure.USART_BaudRate = baudrate;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART3, &USART_InitStructure, HSE_VALUE);
    USART_Cmd(USART3, ENABLE);
    
    // Enable RX interrupt
    USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);
    /**
    * Set Channel to USART3
    * Set Channel Cmd to enable. That will enable USART1 channel in NVIC
    * Set Both priorities to 0. This means high priority
    *
    * Initialize NVIC
    */
    NVIC_InitStruct.NVIC_IRQChannel = USART3_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0;
    NVIC_Init(&NVIC_InitStruct);
    return UART_OK;
}

/**
* @brief  IRQ entrypoint for data from UART3
*/
void USART3_IRQHandler(void)
{
    uint8_t c;
    //Check if we have received data in UART3 peripheral
    //And read them all into buffer
    while(USART_GetITStatus(USART3, USART_IT_RXNE) == SET)
    {
        //Read data from UART3 RX register
        c = (uint8_t)USART_ReceiveData(USART3);
        //Write them into ring buffer on move write pointer +1
        uartBuffer[uartBufferWritePtr] = c;
        uartBufferWritePtr++; //pointers are 8bit long, so they will be always in range
        //Check if we overrun
        if(uartBufferWritePtr == uartBufferReadPtr)
        {
            //yep, we did, reset pointers
            uartBufferOverrun = 1;
            uartBufferWritePtr = 0;
            uartBufferReadPtr = 0;
        }
    }
}
#endif
