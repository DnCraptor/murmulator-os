#include <hardware/pwm.h>
#include <pico/multicore.h>
#include <pico/time.h>
#include "FreeRTOS.h"
#include "task.h"
#include "sound.h"

void blimp(uint32_t count, uint32_t tiks_to_delay) {
    for (uint32_t i = 0; i < count; ++i) {
        vTaskDelay(tiks_to_delay);
        pwm_set_gpio_level(BEEPER_PIN, (1 << 12) - 1);
        vTaskDelay(tiks_to_delay);
        pwm_set_gpio_level(BEEPER_PIN, 0);
    }
}

static repeating_timer_t m_timer = { 0 };
static volatile int16_t* m_buff = NULL;
static volatile uint8_t m_channels = 0;
static volatile size_t m_off = 0; // in 16-bit words
static volatile size_t m_size = 0; // 16-bit values prepared (available)
static volatile pcm_end_callback_t m_cb = NULL;
static volatile bool m_let_process_it = false;

static bool __not_in_flash_func(timer_callback)(repeating_timer_t *rt) { // core#1
    m_let_process_it = true;
    return true;
}

void pcm_call() {
    if (!m_let_process_it) return;
    static uint16_t outL = 0;
    static uint16_t outR = 0;
    pwm_set_gpio_level(PWM_PIN0, outR); // Право
    pwm_set_gpio_level(PWM_PIN1, outL); // Лево
    outL = outR = 0;

    if (!m_channels || !m_buff || m_off >= m_size) {
        m_let_process_it = false;
        return;
    }
    int16_t* b = m_buff + m_off;
    uint16_t x = *b > 0 ? *b : -(*b);
    outL = x >> 4;
    ++m_off;
    if (m_channels == 2) {
        ++b;
        x = *b > 0 ? *b : -(*b);
        outR = x >> 4;
        ++m_off;
    } else {
        outR = outL;
    }
    if (m_cb && m_off >= m_size) {
        m_buff = m_cb(&m_size);
        m_off = 0;
    }
    m_let_process_it = false;
    return;
}

void pcm_cleanup(void) {
    uint16_t o = 0;
    pwm_set_gpio_level(PWM_PIN0, o); // Право
    pwm_set_gpio_level(PWM_PIN1, o); // Лево
    cancel_repeating_timer(&m_timer);
    m_timer.delay_us = 0;
    m_let_process_it = false;
}

void pcm_setup(int hz) {
    if (m_timer.delay_us) pcm_cleanup();
    m_let_process_it = false;
    //hz; // 44100;	//44000 //44100 //96000 //22050
	// negative timeout means exact delay (rather than delay between callbacks)
	add_repeating_timer_us(-1000000 / hz, timer_callback, NULL, &m_timer);
}

void pcm_set_buffer(int16_t* buff, uint8_t channels, size_t size, pcm_end_callback_t cb) {
    m_buff = buff;
    m_channels = channels;
    m_size = size;
    m_cb = cb;
    m_off = 0;
}
