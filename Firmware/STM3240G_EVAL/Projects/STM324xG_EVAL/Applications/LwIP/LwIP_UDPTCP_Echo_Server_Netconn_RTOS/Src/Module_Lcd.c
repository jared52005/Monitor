/*******************************************************************************
 * @file    Module_Lcd.c
 * @brief   Module controlling generic LCD display, which can write lines
 ******************************************************************************
 * @attention
 ******************************************************************************  
 */ 
#include <stdio.h>
#include "stm324xg_eval_lcd.h" 
#include "System_stats.h"
#include "rtos_utils.h"
//******************************************************************************
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "croutine.h"
//******************************************************************************

//private definitions
//private variables
//private functions
void vTaskGetRunTimeStats(char *line);

const char* DeviceTypeTranslate(void)
{
    switch (Stats_UsbActiveDevice_Get())
    {
    case UsbInterface:
        return "USB";
    case UartInterface:
        return "UART";
    case SpiInterface:
        return "SPI";
    case EthernetInterface:
        return "ETH";
    default:
        return "???";
    }
}

/**
* @brief  Task for printing data on LCD. Low prioririty task. Run only once in 100ms
*/
void Application_Lcd_Task(void const* pvParameters)
{
    uint32_t peakCanBusLoad;
    char line[64];
    int lineNumber = -1;
    TickType_t xLastWakeTime;
    const TickType_t xFrequency = 100;

    // Initialise the xLastWakeTime variable with the current time.
    xLastWakeTime = xTaskGetTickCount();
	  peakCanBusLoad = 0;

    for(;;lineNumber++)
    {
        switch (lineNumber)
        {
        case 0:
            sprintf(line, "Time [ms]: %d", GetTime_ms());
            //Using https://www.freertos.org/uxTaskGetSystemState.html is problematic, because when system trace needs to be enabled
            //However firmware will crash on boot when I enable tracing (am I missing some module?)
            //CPU load in my implementation is nonsensical value because I am using state machines combined with TaskYield() everywhere, 
            //processor will be always running 100%. Either 1 task or 100 tasks
            break;
        case 1:
            sprintf(line, "RAM Heap: Total %d [B] Free %d [B]   ", configTOTAL_HEAP_SIZE, xPortGetFreeHeapSize());
            break;
        case 2:
            sprintf(line, "CAN: TX Msgs %d  RX Msgs %d   ", Stats_CanMessages_TxTotal_Get(), Stats_CanMessages_RxTotal_Get());
            break;
        case 3:
            sprintf(line, "CAN: RX %d kB;  %d kBps   ", 
            Stats_CanBytes_RxTotal_Get() / 1024, 
            Stats_CanBytes_RxPerSecond_Get() / 1024);
            break;
        case 4:
            sprintf(line, "CAN Bus Load: %d %%  ", Stats_CanMessages_BusLoad_Get());
            break;
        case 5:
            if(Stats_CanMessages_BusLoad_Get() > peakCanBusLoad)
            {
                peakCanBusLoad = Stats_CanMessages_BusLoad_Get();
            }
            sprintf(line, "CAN Bus Peak Load: %d %%  ", peakCanBusLoad);
            break;
        case 6:
            sprintf(line, "%s: TX Msgs %d  RX Msgs %d   ", 
            DeviceTypeTranslate(),  
            Stats_UsbMessages_TxTotal_Get(), 
            Stats_UsbMessages_RxTotal_Get());
            break;
        case 7:
            sprintf(line, "%s: TX %d kB  %d kBps   ", 
            DeviceTypeTranslate(),  
            (uint32_t)(Stats_UsbBytes_TxTotal_Get() / 1024), 
            (uint32_t)(Stats_UsbBytes_TxPerSecond_Get() / 1024));
            break;
        case 8:
            sprintf(line, "%s: RX %d kB  %d kBps   ", 
            DeviceTypeTranslate(),  
            (uint32_t)(Stats_UsbBytes_RxTotal_Get() / 1024), 
            (uint32_t)(Stats_UsbBytes_RxPerSecond_Get() / 1024));
            break;
        default:
            lineNumber = -1;
            // Wait for the next cycle.
            vTaskDelayUntil( &xLastWakeTime, xFrequency );
            continue;
        }
        BSP_LCD_DisplayStringAtLine(lineNumber, (uint8_t*)line);
    }
}

/**
* @brief  Setup LCD
*/
void Application_Lcd_Init(void)
{
    if(BSP_LCD_Init() == LCD_OK)
    {
        BSP_LCD_DisplayOn();
        /* Clear the LCD Background layer */
        BSP_LCD_Clear(LCD_COLOR_BLACK);
        printf("LCD: Init success\n");
    }
    else
    {
        printf("LCD: Init FAILED\n");
    }
}
