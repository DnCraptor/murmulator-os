#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define RGB888(r, g, b) ((r<<16) | (g << 8 ) | b )

void vga_init(void);

void vga_driver_init(void);
void vga_cleanup(void);
uint32_t get_vga_console_width(void);
uint32_t get_vga_console_height(void);
uint8_t* get_vga_buffer(void);
void set_vga_buffer(uint8_t* buffer);
void vga_clr_scr(const uint8_t color);
void vga_draw_text(const char* string, int x, int y, uint8_t color, uint8_t bgcolor);
uint8_t get_vga_buffer_bitness(void);
void vga_set_bgcolor(const uint32_t color888);
size_t vga_buffer_size();
void vga_set_con_pos(int x, int y);
int vga_con_x(void);
int vga_con_y(void);
void vga_set_con_color(uint8_t color, uint8_t bgcolor);
void vga_print(char* buf);
void vga_backspace(void);
int vga_get_mode(void);
bool vga_set_mode(int mode);
void vga_lock_buffer(bool);
bool vga_is_text_mode();
bool vga_is_mode_text(int mode);
typedef uint8_t* (*dma_handler_impl_fn)(void);
void set_vga_dma_handler_impl(dma_handler_impl_fn impl);
void set_vga_clkdiv(uint32_t pixel_clock, uint32_t line_size);
