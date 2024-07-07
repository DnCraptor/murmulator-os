#pragma once
#include "stdbool.h"

#define PIO_VGA (pio0)
#ifndef VGA_BASE_PIN
#define VGA_BASE_PIN (6)
#endif
#define VGA_DMA_IRQ (DMA_IRQ_0)

#define RGB888(r, g, b) ((r<<16) | (g << 8 ) | b )

void vga_init(void);
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
