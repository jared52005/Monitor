#include "SystemIf.h"
#include "UsbIf.h"
#include "CanIf.h"
#include "UartIf.h"
#include "Usb_Vcp_If.h"
#include "led.h"
#include "application.h"
#include "GpioIf.h"
#include "GptIf.h"

#include "CanDriver.h"

#include "string.h"
#include "stdbool.h"

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "croutine.h"
#include "rtos_utils.h"
//******************************************************************************

#ifdef DEBUG_TRACE
#include "stdio.h"
#endif

char _cmdRxPartialBuffer[128];
char _cmd[2048];

void ProcessCommand(uint16_t cmdSize);

/**
 * @brief Common method which is running main loop for all existing devices
*/
void Application(void)
{
    uint16_t bufferRxPartialSize;
    uint16_t cmdSize;
    bool cmdReceived;
    int i;
    /* Initialize all configured peripherals */
    TM_USB_VCP_Init();
    led_init();
    Can_Mode_Set(CAN_ACTIVE);
	Can_Baudrate_Set(CAN_BITRATE_500K);
    if (Can_Enable() != ERROR_OK)
	{
        printf("ERROR: Init of CAN peripheral has failed\r\n");
    }
	
    //Initialize CAN Raw Loopback
    xTaskCreate(CanRawLoopback_Task, (const char*)"CAN Raw Loopback", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);

    // blink red LED for test
    GPIO_WritePin(LED_RED, GPIO_PIN_SET_);
    Wait_ms(100);
    GPIO_WritePin(LED_RED, GPIO_PIN_RESET_);
    Wait_ms(100);
    GPIO_WritePin(LED_RED, GPIO_PIN_SET_);
    Wait_ms(100);
    GPIO_WritePin(LED_RED, GPIO_PIN_RESET_);
	Wait_ms(100);
    GPIO_WritePin(LED_RED, GPIO_PIN_SET_); //Let red LED on

#ifdef DEBUG_TRACE
    printf("SLCAN - Boot finished\r\n");
#endif
    cmdSize = 0;
    for(;;)
    {
        taskYIELD();
        //Get partial command buffer from USB
		bufferRxPartialSize = TM_USB_VCP_Gets(_cmdRxPartialBuffer, 128);
        if(bufferRxPartialSize != 0)
        {
            //Copy partial buffer into overal buffer
            memcpy((_cmd + cmdSize), _cmdRxPartialBuffer, bufferRxPartialSize);
            cmdSize += bufferRxPartialSize;
            //Check if we have received all characters
            cmdReceived = false;
            for(i = 0; i<cmdSize; i++)
            {
                if(_cmd[i] == '\n')
                {
                    cmdReceived = true;
                    cmdSize = i;
                }
            }
            if(cmdReceived == true)
            {
                //Loopback
                //TM_USB_VCP_Puts(cmd);
                ProcessCommand(cmdSize);
                cmdSize = 0;
            }
        }
    }
}

void ProcessCommand(uint16_t cmdSize)
{
    bool validCommand = true;
    if(strstr(_cmd, "Command") != NULL)
    {
        //Do a command
    }
    else if(_cmd[0] == 'D')
    {
        //Setup bootflag and request reset
        *((uint32_t*)0x20000000) = 0x44465542; //"DFUB"
        System_Reset();
    }
    else
    {
        validCommand = false;
    }
    if(validCommand)
    {
        TM_USB_VCP_Puts("ok\n");
    }
    else
    {
        TM_USB_VCP_Puts("ERROR: invalid command\n");
    }
}
