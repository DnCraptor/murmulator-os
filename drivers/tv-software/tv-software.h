#pragma once

#include "inttypes.h"
#include "stdbool.h"

#define PIO_VIDEO pio0

#define TV_BASE_PIN (6)

#define TEXTMODE_COLS 80
#define TEXTMODE_ROWS 30
#define RGB888(r, g, b) ((r<<16) | (g << 8 ) | b )

typedef enum g_out_TV_t {
    g_TV_OUT_PAL,
    g_TV_OUT_NTSC
} g_out_TV_t;


typedef enum NUM_TV_LINES_t {
    _624_lines, _625_lines, _524_lines, _525_lines,
} NUM_TV_LINES_t;

typedef enum COLOR_FREQ_t {
    _3579545, _4433619
} COLOR_FREQ_t;

// TODO: Сделать настраиваемо
extern const uint8_t textmode_palette[16];

int stv_get_default_mode(void);
void stv_driver_init(void);
void stv_cleanup(void);
void stv_set_mode(int mode);
bool stv_is_mode_text(int mode);
bool stv_is_text_mode();
int stv_get_mode(void);
uint32_t stv_console_width(void);
uint32_t stv_console_height(void);
uint8_t* get_stv_buffer(void);
void set_stv_buffer(uint8_t*);
void stv_clr_scr(const uint8_t color);
uint8_t get_stv_buffer_bitness(void);
void stv_set_offset(int x, int y);
void stv_set_bgcolor(uint32_t color888); //определяем зарезервированный цвет в палитре
size_t stv_buffer_size();
void stv_lock_buffer(bool b);
void stv_set_cursor_color(uint8_t color);
