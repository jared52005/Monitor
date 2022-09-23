/* Simple HTTP Server Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <esp_log.h>
#include <esp_system.h>
#include <sys/param.h>

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "driver/uart.h"
#include "driver/gpio.h"

#include "uart.h"


#define TAG "uart.c"

static const int RX_BUF_SIZE = 1024;

#define TXD1_PIN (GPIO_NUM_38)
#define RXD1_PIN (GPIO_NUM_37)

ErrorCodes Uart_Enable(void) 
{
    const uart_config_t uart_config = {
        .baud_rate = 10400,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    // We won't use a buffer for sending data.
    uart_driver_install(UART_NUM_1, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, TXD1_PIN, RXD1_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    return ERROR_OK;
}

ErrorCodes Uart_Rx(uint8_t* c)
{
    const int rxBytes = uart_read_bytes(UART_NUM_1, c, 1, 1000 / portTICK_RATE_MS);
    if (rxBytes > 0) 
    {
        //ESP_LOGI(TAG, "Read %x ", *c);
    }
    return ERROR_OK;
}
