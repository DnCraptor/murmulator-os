#pragma once

#include "stdbool.h"

#define PIO_VIDEO pio0
#define PIO_VIDEO_ADDR pio0

#define TV_BASE_PIN (6)

#define TEXTMODE_COLS 53
#define TEXTMODE_ROWS 30

#define RGB888(r, g, b) ((r<<16) | (g << 8 ) | b )

typedef enum {
    TV_OUT_PAL,
    TV_OUT_NTSC
} output_format_e;

int tv_get_default_mode(void);
void tv_driver_init(void);
void tv_cleanup(void);
void tv_set_mode(int mode);
bool tv_is_mode_text(int mode);
bool tv_is_text_mode();
int tv_get_mode(void);
uint32_t tv_console_width(void);
uint32_t tv_console_height(void);
uint8_t* get_tv_buffer(void);
void set_tv_buffer(uint8_t*);
void tv_clr_scr(const uint8_t color);
uint8_t get_tv_buffer_bitness(void);
void tv_set_offset(int x, int y);
void tv_set_bgcolor(uint32_t color888); //определяем зарезервированный цвет в палитре
size_t tv_buffer_size();
void tv_lock_buffer(bool b);
void tv_set_cursor_color(uint8_t color);
