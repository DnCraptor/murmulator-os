#include <hardware/pwm.h>
#include "FreeRTOS.h"
#include "task.h"

void blimp(uint32_t count, uint32_t tiks_to_delay) {
    for (uint32_t i = 0; i < count; ++i) {
        vTaskDelay(tiks_to_delay);
        pwm_set_gpio_level(BEEPER_PIN, (1 << 12) - 1);
        vTaskDelay(tiks_to_delay);
        pwm_set_gpio_level(BEEPER_PIN, 0);
    }
}
