/*******************************************************************************
 * @brief   Reading data from peripherals, pushing them through parsers and
 *          writing them into TCP buffers for sending
 ******************************************************************************
 * @attention
 ******************************************************************************  
 */ 

#include <stdint.h>

/**
* @brief  Task for printing data on LCD. Low prioririty task. Run only once in 100ms
*/
void Task_Hub(void const* pvParameters);
