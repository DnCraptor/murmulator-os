#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "inttypes.h"
#include "stdbool.h"

#include "hardware/pio.h"

#define PIO_VIDEO pio0
#define PIO_VIDEO_ADDR pio0
#define VIDEO_DMA_IRQ (DMA_IRQ_0)

#ifndef HDMI_BASE_PIN
#define HDMI_BASE_PIN (6)
#endif

#if ZERO
#define HDMI_PIN_RGB_notBGR (0)
#define HDMI_PIN_invert_diffpairs (0)
#define beginHDMI_PIN_data (HDMI_BASE_PIN)
#define beginHDMI_PIN_clk (HDMI_BASE_PIN+6)
#else
#define HDMI_PIN_RGB_notBGR (1)
#define HDMI_PIN_invert_diffpairs (1)
#define beginHDMI_PIN_data (HDMI_BASE_PIN+2)
#define beginHDMI_PIN_clk (HDMI_BASE_PIN)
#endif

#define TEXTMODE_COLS 53
#define TEXTMODE_ROWS 30

#define RGB888(r, g, b) ( (r << 16) | (g << 8) | b )

extern const uint8_t textmode_palette[16];

int hdmi_get_default_mode(void);
void hdmi_driver_init(void);
void hdmi_cleanup(void);
void hdmi_set_mode(int mode);
bool hdmi_is_mode_text(int mode);
bool hdmi_is_text_mode();
int hdmi_get_mode(void);
uint32_t hdmi_console_width(void);
uint32_t hdmi_console_height(void);
uint32_t hdmi_screen_width(void);
uint32_t hdmi_screen_height(void);
uint8_t* get_hdmi_buffer(void);
void set_hdmi_buffer(uint8_t*);
void hdmi_clr_scr(const uint8_t color);
uint8_t get_hdmi_buffer_bitness(void);
void hdmi_set_offset(int x, int y);
void hdmi_set_bgcolor(uint32_t color888); //определяем зарезервированный цвет в палитре
size_t hdmi_buffer_size();
void hdmi_lock_buffer(bool b);
void hdmi_set_cursor_color(uint8_t color);

#ifdef __cplusplus
}
#endif
