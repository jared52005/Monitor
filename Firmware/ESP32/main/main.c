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

#include "driver/gpio.h"

#include "CanIf.h"
#include "uart.h"

#define LED_GPIO 27

#define TAG "main.c"

void app_main(void)
{
    //Configure LED
    gpio_reset_pin(LED_GPIO);
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);

	//config_uart();
	Can_Enable(500000, CAN_PASSIVE);

    ESP_LOGE(TAG, "Error Example");
    ESP_LOGW(TAG, "Warning Example");
    ESP_LOGI(TAG, "Info Example");

    for(;;)
    {
        //Blink with LED
        gpio_set_level(LED_GPIO, 0);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        gpio_set_level(LED_GPIO, 1);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
