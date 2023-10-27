#include <stdint.h>
#include <stdbool.h>
#include "ErrorCodes.h"

typedef enum
{
    PWM_CHANNEL_01,
}PwmName;

void Pwm_Enable(PwmName name, uint32_t freq, uint32_t dc1, uint32_t dc2);
