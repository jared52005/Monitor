#include <stdint.h>
#include <stdbool.h>
#include "CanIf.h"

/**
* @brief  Thread in which we are refreshing CAN state machine
*/
void CanDriver_Task(void* arg);
/**
 * @brief Enqueue data for transmission
 */
void CanDriver_Transmit(CanMessage cmsg);
/**
 * @brief Get enqueued data.
 * @return True if message was dequeued. False if there is no message available.
 */
bool CanDriver_Receive(CanMessage* cmsg);
/**
 * @brief Start CAN driver task
*/
void CanDriver_Start(void);
/**
 * @brief Stop CAN driver task
*/
void CanDriver_Stop(void);
