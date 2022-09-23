/*******************************************************************************
 * @brief   Reading data from peripherals, pushing them through parsers and
 *          writing them into TCP buffers for sending
 ******************************************************************************
 * @attention
 ******************************************************************************  
 */ 

#include <stdint.h>

/**
* @brief  Task for reading bytes from UART interface and writing them into TCP stream
*/
void Task_KlineReconstruct(void* pvParameters);
