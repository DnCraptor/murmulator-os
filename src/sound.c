#include <hardware/pwm.h>
#include <pico/multicore.h>
#include <pico/stdlib.h>
#include <pico/time.h>
#include "FreeRTOS.h"
#include "task.h"
#include "sound.h"
#include "audio.h"

#define SOUND_FREQUENCY 44100

#ifdef I2S_SOUND
/*
 * I2S output runs continuously at a fixed SOUND_FREQUENCY, whatever the WAV rate is.
 * PCM5102A with SCK tied to GND derives its clock from BCK (PLL mode) and does not
 * lock at low sample rates with 32*fs BCK (e.g. 8000 Hz test files), so the rate is
 * never switched. Samples are taken by the same timer/pcm_call() path as for PWM and
 * held (zero-order hold) until the next one: core#1 keeps the PIO TX FIFO full.
 */
static volatile bool m_i2s_ready = false;
static volatile int16_t m_beep = 0;
static uint32_t m_frame = 0; // core#1 only: low 16 bits - left, high 16 bits - right
static i2s_config_t i2s_config = {
		.sample_freq = SOUND_FREQUENCY, 
		.channel_count = 2,
		.data_pin = PWM_PIN0,
		.clock_pin_base = PWM_PIN1,
#if NUM_PIOS > 2
		.pio = pio2, // RP2350: own PIO, as in murm386 (pio1 is shared with PS/2 keyboard and NES gamepad)
#else
		.pio = pio1,
#endif
		.sm = 0,
        .dma_channel = 0,
        .dma_buf = NULL,
        .dma_trans_count = 0,
        .volume = 0,
	};
#else
static void PWM_init_pin(uint8_t pinN, uint16_t max_lvl) {
    pwm_config config = pwm_get_default_config();
    gpio_set_function(pinN, GPIO_FUNC_PWM);
    pwm_config_set_clkdiv(&config, 1.0);
    pwm_config_set_wrap(&config, max_lvl); // MAX PWM value
    pwm_init(pwm_gpio_to_slice_num(pinN), &config, true);
}
#endif

inline static void inInit(uint gpio) {
    gpio_init(gpio);
    gpio_set_dir(gpio, GPIO_IN);
    gpio_pull_up(gpio);
}

void init_sound() {
#ifdef I2S_SOUND
    i2s_config.sample_freq = SOUND_FREQUENCY;
    i2s_config.dma_trans_count = 0; // DMA is not used, samples are pushed by pcm_call()
    i2s_volume(&i2s_config, 0);
    i2s_init(&i2s_config);
    PIO pio = i2s_config.pio;
    uint sm = i2s_config.sm;
    pio_sm_set_enabled(pio, sm, false);
    hw_set_bits(&pio->sm[sm].shiftctrl, PIO_SM0_SHIFTCTRL_FJOIN_TX_BITS); // 8-word TX FIFO
    while (!pio_sm_is_tx_fifo_full(pio, sm)) {
        pio_sm_put(pio, sm, 0);
    }
    pio_sm_set_enabled(pio, sm, true);
    m_i2s_ready = true;
#else
    PWM_init_pin(PWM_PIN0, (1 << 12) - 1);
    PWM_init_pin(PWM_PIN1, (1 << 12) - 1);
    PWM_init_pin(BEEPER_PIN, (1 << 12) - 1);
#endif
#ifdef WAV_IN_PIO
    //пин ввода звука
    inInit(WAV_IN_PIO);
#endif
}

void blimp(uint32_t count, uint32_t tiks_to_delay) {
#ifndef I2S_SOUND
    for (uint32_t i = 0; i < count; ++i) {
        vTaskDelay(tiks_to_delay);
        pwm_set_gpio_level(BEEPER_PIN, (1 << 12) - 1);
        vTaskDelay(tiks_to_delay);
        pwm_set_gpio_level(BEEPER_PIN, 0);
    }
#else
    for (uint32_t i = 0; i < count; ++i) {
        vTaskDelay(tiks_to_delay);
        m_beep = 0x2000;
        vTaskDelay(tiks_to_delay);
        m_beep = 0;
    }
#endif
}

static repeating_timer_t m_timer = { 0 };
static volatile pcm_end_callback_t m_cb = NULL;
static volatile int16_t* m_buff = NULL;
static volatile uint8_t m_channels = 0;
static volatile size_t m_off = 0; // in 16-bit words
static volatile size_t m_size = 0; // 16-bit values prepared (available)
static volatile bool m_let_process_it = false;

static bool __not_in_flash_func(timer_callback)(repeating_timer_t *rt) { // core#1?
    m_let_process_it = true;
    return true;
}

void pcm_call() {
#ifdef I2S_SOUND
    if (!m_i2s_ready) {
        return;
    }
    PIO pio = i2s_config.pio;
    uint sm = i2s_config.sm;
    while (!pio_sm_is_tx_fifo_full(pio, sm)) {
        pio_sm_put(pio, sm, m_frame);
    }
    if (!m_channels) { // no PCM playback, only beeper
        uint16_t b = (uint16_t)m_beep;
        m_frame = ((uint32_t)b << 16) | b;
        return;
    }
    if (!m_let_process_it) {
        return;
    }
    int16_t l = 0;
    int16_t r = 0;
    if (m_buff && m_off < m_size) {
        volatile int16_t* b = m_buff + m_off;
        l = *b;
        ++m_off;
        if (m_channels == 2) {
            ++b;
            r = *b;
            ++m_off;
        } else {
            r = l;
        }
        if (m_cb && m_off >= m_size) {
            m_buff = (volatile int16_t*)m_cb((size_t*)&m_size);
            m_off = 0;
        }
    }
    m_frame = ((uint32_t)(uint16_t)r << 16) | (uint16_t)l;
    m_let_process_it = false;
#else
    if (!m_let_process_it) {
        return;
    }
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
    uint32_t x = ((int32_t)*b) + 0x8000;
    outL = x >> 4;
    ++m_off;
    if (m_channels == 2) {
        ++b;
        x = ((int32_t)*b) + 0x8000;
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
#endif
    return;
}

void pcm_cleanup(void) {
    cancel_repeating_timer(&m_timer);
    m_timer.delay_us = 0;
#ifdef I2S_SOUND
    // I2S keeps running (keeps DAC locked); core#1 outputs silence or beeper level
    m_channels = 0;
    m_let_process_it = false;
#else
    uint16_t o = 0;
    pwm_set_gpio_level(PWM_PIN0, o); // Право
    pwm_set_gpio_level(PWM_PIN1, o); // Лево
    m_let_process_it = false;
#endif
}

void pcm_setup(int hz) {
    if (m_timer.delay_us) {
        pcm_cleanup();
    }
    m_let_process_it = false;
    //hz; // 44100;	//44000 //44100 //96000 //22050
	// negative timeout means exact delay (rather than delay between callbacks)
	add_repeating_timer_us(-1000000 / hz, timer_callback, NULL, &m_timer);
}

// size - in 16-bit values count
void pcm_set_buffer(int16_t* buff, uint8_t channels, size_t size, pcm_end_callback_t cb) {
    m_cb = cb;
    m_size = 0;
    m_buff = buff;
    m_channels = channels;
    m_off = 0;
    m_size = size;
}
