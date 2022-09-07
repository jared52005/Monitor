/* TWAI Network Slave Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

/*
 * The following example demonstrates a slave node in a TWAI network. The slave
 * node is responsible for sending data messages to the master. The example will
 * execute multiple iterations, with each iteration the slave node will do the
 * following:
 * 1) Start the TWAI driver
 * 2) Listen for ping messages from master, and send ping response
 * 3) Listen for start command from master
 * 4) Send data messages to master and listen for stop command
 * 5) Send stop response to master
 * 6) Stop the TWAI driver
 */

#include <stdio.h>
#include <stdlib.h>
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/twai.h"

#include "CanIf.h"
#include "rtos_utils.h"

/* --------------------- Definitions and static variables ------------------ */
//Example Configuration
#define DATA_PERIOD_MS                  50
#define NO_OF_ITERS                     3
#define ITER_DELAY_MS                   1000
#define RX_TASK_PRIO                    8       //Receiving task priority
#define TX_TASK_PRIO                    9       //Sending task priority
#define CTRL_TSK_PRIO                   10      //Control task priority
#define TX_GPIO_NUM                     (GPIO_NUM_5)
#define RX_GPIO_NUM                     (GPIO_NUM_4)
#define TAG                             "CanIf.c"

static const twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(TX_GPIO_NUM, RX_GPIO_NUM, TWAI_MODE_LISTEN_ONLY);
static const twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
static const twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

/* --------------------------- Tasks and Functions -------------------------- */
#if false
static void twai_receive_task(void *arg)
{
    esp_err_t result;
    twai_message_t rx_msg;

    while (1) 
    {
         result = twai_receive(&rx_msg, portMAX_DELAY);
         if (result == ESP_OK) 
         {
             ESP_LOGI(pcTaskGetTaskName(0),"RX ID=0x%x flags=0x%x-%x-%x DLC=%d", rx_msg.identifier, rx_msg.flags, rx_msg.extd, rx_msg.rtr, rx_msg.data_length_code);
             ESP_LOG_BUFFER_HEXDUMP(TAG, rx_msg.data, rx_msg.data_length_code, ESP_LOG_INFO);
         }
    }
}
#endif

ErrorCodes Can_Rx(CanMessage *msg)
{
    esp_err_t result;
    twai_message_t rx_msg;
    result = twai_receive(&rx_msg, portMAX_DELAY);
    if (result == ESP_OK) 
    {
        //ESP_LOGI(pcTaskGetTaskName(0),"RX ID=0x%x flags=0x%x-%x-%x DLC=%d", rx_msg.identifier, rx_msg.flags, rx_msg.extd, rx_msg.rtr, rx_msg.data_length_code);
        //ESP_LOG_BUFFER_HEXDUMP(TAG, rx_msg.data, rx_msg.data_length_code, ESP_LOG_INFO);

        msg->Dlc = rx_msg.data_length_code;
        msg->Id = rx_msg.identifier;
        memcpy(msg->Frame, rx_msg.data, rx_msg.data_length_code);
        msg->Timestamp = GetTime_ms();
        return ERROR_OK;
    }
    return ERROR_DATA_EMPTY;
}

ErrorCodes Can_Enable(uint32_t baudrate, CanMode mode)
{
    //Install TWAI driver, trigger tasks to start
    ESP_ERROR_CHECK(twai_driver_install(&g_config, &t_config, &f_config));
    ESP_LOGI(TAG, "CAN Driver installed");
    ESP_ERROR_CHECK(twai_start());
    ESP_LOGI(TAG, "CAN Driver started");

    //xTaskCreatePinnedToCore(twai_receive_task, "TWAI_rx", 4096, NULL, RX_TASK_PRIO, NULL, tskNO_AFFINITY);
    return ERROR_OK;
}
