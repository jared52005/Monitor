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
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/twai.h"

#include "can.h"

#define TWAI_FILTER_CONFIG_ACCEPT_UDS_7E0() {.acceptance_code = (0x7E8 << 21), .acceptance_mask = ~(0x7FF << 21), .single_filter = true}

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
#define EXAMPLE_TAG                     "CAN/TWAI"

#define ID_SLAVE_PING_RESP              0x7E0

static const twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(TX_GPIO_NUM, RX_GPIO_NUM, TWAI_MODE_NORMAL);
static const twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
static const twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_UDS_7E0();
static const twai_message_t can_msg = {.identifier = ID_SLAVE_PING_RESP, .data_length_code = 8,
                                        .data = {0x02, 0x3E , 0x00 , 0xAA ,0xAA ,0xAA ,0xAA, 0xAA}};

/* --------------------------- Tasks and Functions -------------------------- */

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
             ESP_LOG_BUFFER_HEXDUMP(EXAMPLE_TAG, rx_msg.data, rx_msg.data_length_code, ESP_LOG_INFO);
         }
    }
}

static void twai_transmit_task(void *arg)
{
    esp_err_t ret;
    while (1) 
    {
        //Transmit ping response to master
        ret = twai_transmit(&can_msg, portMAX_DELAY);
        if(ret == ESP_OK) 
        {
            ESP_LOGI(EXAMPLE_TAG, "TX DONE");
        }
        else 
        {
            ESP_LOGE(EXAMPLE_TAG, "CAN ERROR = %d", ret);
        }
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

void config_can(void)
{
    //Install TWAI driver, trigger tasks to start
    ESP_ERROR_CHECK(twai_driver_install(&g_config, &t_config, &f_config));
    ESP_LOGI(EXAMPLE_TAG, "CAN Driver installed");
    ESP_ERROR_CHECK(twai_start());
    ESP_LOGI(EXAMPLE_TAG, "CAN Driver started");

    xTaskCreatePinnedToCore(twai_receive_task, "TWAI_rx", 4096, NULL, RX_TASK_PRIO, NULL, tskNO_AFFINITY);
    xTaskCreatePinnedToCore(twai_transmit_task, "TWAI_tx", 4096, NULL, TX_TASK_PRIO, NULL, tskNO_AFFINITY);
}
