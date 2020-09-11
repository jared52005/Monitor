/*******************************************************************************
 * @brief   Reading data from peripherals, pushing them through parsers and
 *          writing them into TCP buffers for sending
 ******************************************************************************
 * @attention
 ******************************************************************************  
 */ 

#include <stdio.h>
#include "main.h"
#include "System_stats.h"
#include "rtos_utils.h"
//******************************************************************************
//#include "FreeRTOS.h"
//#include "queue.h"
//#include "task.h"
//#include "croutine.h"

/**
* @brief  Task for printing data on LCD. Low prioririty task. Run only once in 100ms
*/
void Task_Hub(void const* pvParameters)
{
    //Init CAN
    for(;;)
    {
        BSP_LED_Toggle(LED2);
        osDelay(333);
    }
}
