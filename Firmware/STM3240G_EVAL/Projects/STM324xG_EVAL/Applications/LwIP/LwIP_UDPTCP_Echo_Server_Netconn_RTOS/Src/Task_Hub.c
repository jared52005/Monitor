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
#include "CanIf.h"
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
    ErrorCodes error;
    error = Can_Enable(500000, CAN_ACTIVE);
    printf("CAN Setup result: %d\n", error);
    //Init CAN
    for(;;)
    {
        BSP_LED_Toggle(LED2);
        osDelay(333);
    }
}
