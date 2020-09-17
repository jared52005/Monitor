/*******************************************************************************
  * @brief   Control of default ILI9525 LCD
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

const char* TranslateDhcpState(uint32_t dhcpState)
{
    switch (dhcpState)
    {
    case 0:
    return "Off";
    case 1:
    return "Start";
    case 2:
    return "Waiting on IP...";
    case 3:
    return "IP assigned";
    case 4:
    return "Timeout";
    case 5:
    return "Link Down!";
    default:
    return "???";
    }
}

const char* TranslateSocketState(uint32_t state)
{
    switch (state)
    {
    case 0:
    return "Disconnected";
    case 1:
    return "Connected   ";
    default:
    return "???";
    }
}

/**
* @brief  Task for printing data on LCD. Low prioririty task. Run only once in 100ms
*/
void Task_Lcd(void const* pvParameters)
{
    uint32_t time_ms;
    uint32_t time_sec;
    uint32_t time_min;
    uint32_t dhcpState;
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
            time_ms = GetTime_ms() % 1000;
            time_sec = GetTime_ms() / 1000;
            time_min = time_sec / 60;
            time_sec = time_sec % 60;
            sprintf(line, "Time : %d:%02d.%03d   ", time_min, time_sec, time_ms);
            break;
        case 1:
            sprintf(line, "RAM Heap: Total %d [B] Free %d [B]   ", configTOTAL_HEAP_SIZE, xPortGetFreeHeapSize());
            break;
        case 2:
            dhcpState = Stats_DHCP_GetState();
            if(dhcpState != 3)
            {
                sprintf(line, "DHCP: %s", TranslateDhcpState(dhcpState));
            }
            else
            {
                sprintf(line, "IP address: %s   ", Stats_IP_Get());
            }
            break;
        case 3:
            //Empty line
            continue;
        case 4:
            sprintf(line, "CAN: RX Msgs %d  ", Stats_CanMessages_RxTotal_Get());
            break;
        case 5:
            sprintf(line, "CAN: RX %d kB;  %d kBps   ", 
            Stats_CanBytes_RxTotal_Get() / 1024, 
            Stats_CanBytes_RxPerSecond_Get() / 1024);
            break;
        case 6:
            sprintf(line, "CAN Bus Load: %d %%  ", Stats_CanMessages_BusLoad_Get());
            break;
        case 7:
            if(Stats_CanMessages_BusLoad_Get() > peakCanBusLoad)
            {
                peakCanBusLoad = Stats_CanMessages_BusLoad_Get();
            }
            sprintf(line, "CAN Bus Peak Load: %d %%  ", peakCanBusLoad);
            break;
        case 8:
            //Empty line
            continue;
        case 9:
            sprintf(line, "KLINE: Baudrate %d  ", Stats_Kline_GetBaudrate());
            break;
        case 10:
            sprintf(line, "KLINE: RX Frames %d  ", Stats_KlineFrames_RxTotal_Get());
            break;
        case 11:
            sprintf(line, "KLINE: RX %d kB;  %d Bps   ", 
            Stats_KlineBytes_RxTotal_Get() / 1024, 
            Stats_KlineBytes_RxPerSecond_Get());
            break;
        case 12:
            //Empty line
            continue;
        case 13:
            sprintf(line, "TCP Socket CAN: %s  ", TranslateSocketState(Stats_TCP_WS_SocketCAN_State_Get()));
            break;
        case 14:
            sprintf(line, "TCP Datagrams: %s  ", TranslateSocketState(Stats_TCP_WS_RAW_State_Get()));
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
void Task_Lcd_Init(void)
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
