#include <hardware/pwm.h>
#include <pico/time.h>
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

static repeating_timer_t m_timer = { 0 };
static int16_t* m_buff = NULL;
static uint8_t m_channels = 0;
static size_t m_off = 0; // in 16-bit words
static size_t m_size = 0; // 16-bit values prepared (available)

static bool __not_in_flash_func(timer_callback)(repeating_timer_t *rt) {
    static int16_t outL = 0;
    static int16_t outR = 0;
    pwm_set_gpio_level(PWM_PIN0, outR); // Право
    pwm_set_gpio_level(PWM_PIN1, outL); // Лево
    outL = outR = 0;

    if (!m_channels || !m_buff || m_off >= m_size) {
        return true;
    }
    int16_t* b = m_buff + m_off;
    outL = (*b++) >> 4;
    ++m_off;
    if (m_channels == 2) {
        outR = (*b) >> 4;
        ++m_off;
    } else {
        outR = outL;
    }
    return true;
}

void pcm_cleanup(void) {
    cancel_repeating_timer(&m_timer);
    m_timer.delay_us = 0;
}

void pcm_setup(int hz) {
    if (m_timer.delay_us) pcm_cleanup();
//	int hz = 44100;	//44000 //44100 //96000 //22050
	// negative timeout means exact delay (rather than delay between callbacks)
	return add_repeating_timer_us(-1000000 / hz, timer_callback, NULL, &m_timer);
}

void pcm_set_buffer(int16_t* buff, uint8_t channels, size_t size) {
    m_buff = buff;
    m_channels = channels;
    m_size = size;
}
