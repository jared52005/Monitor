#include <stdint.h>
#include "led.h"
#include "SystemIf.h"
#include "GpioIf.h"
#include "rtos_utils.h"

static uint32_t led_laston = 0;
static uint32_t led_lastoff = 0;

/**
 * @brief Init LED used for state visualization
 */
void led_init(void)
{
    GPIO_InitPin(LED_RED);
    GPIO_InitPin(LED_GREEN);
    GPIO_InitPin(LED_BLUE);
}

/**
 * @brief Attempt to turn on status LED
 */
void led_on(void)
{
    // Make sure the LED has been off for at least LED_DURATION before turning on again
    // This prevents a solid status LED on a busy canbus
    if(led_laston == 0 && GetTime_ms() - led_lastoff > LED_DURATION)
    {
       GPIO_WritePin(LED_RED, GPIO_PIN_SET_);
       led_laston = GetTime_ms();
    }
}

/**
* @brief Process time-based LED events
*/
void led_process(void)
{
    // If LED has been on for long enough, turn it off
    if(led_laston > 0 && GetTime_ms() - led_laston > LED_DURATION)
    {
       //GPIO_WritePin(LED_RED, GPIO_PIN_RESET_);
       led_laston = 0;
       led_lastoff = GetTime_ms();
    }
}

