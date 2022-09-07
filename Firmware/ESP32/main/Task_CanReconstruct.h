/*******************************************************************************
 * @brief   Reading data from peripherals, pushing them through parsers and
 *          writing them into TCP buffers for sending
 ******************************************************************************
 * @attention
 ******************************************************************************  
 */ 

#include <stdint.h>

/**
* @brief  Task for reading CAN messages from TWAI interface and writing them into TCP stream
*/
void Task_CanReconstruct(void const* pvParameters);
