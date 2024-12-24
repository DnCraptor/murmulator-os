#pragma once


#ifndef TFT_RST_PIN
#define TFT_RST_PIN 8
#endif


#ifndef TFT_CS_PIN
#define TFT_CS_PIN 6
#endif


#ifndef TFT_LED_PIN
#define TFT_LED_PIN 9
#endif


#ifndef TFT_CLK_PIN
#define TFT_CLK_PIN 13
#endif 

#ifndef TFT_DATA_PIN
#define TFT_DATA_PIN 12
#endif

#ifndef TFT_DC_PIN
#define TFT_DC_PIN 10
#endif

#define TEXTMODE_COLS 53
#define TEXTMODE_ROWS 30

typedef uint8_t* (*dma_handler_impl_fn)(void);
void set_vga_dma_handler_impl(dma_handler_impl_fn impl);
void vga_dma_channel_set_read_addr(const volatile void* addr);
void set_vga_clkdiv(uint32_t pixel_clock, uint32_t line_size);
void tft_cleanup(void);
bool tft_set_mode(int mode);
bool tft_is_text_mode();

uint32_t get_tft_console_width();
uint32_t get_tft_console_height(void);
uint32_t get_tft_screen_width(void);
uint32_t get_tft_screen_height(void);
uint8_t get_tft_buffer_bitness(void);
uint8_t* get_tft_buffer(void);
void set_tft_buffer(uint8_t *);
void tft_clr_scr(void);
void tft_set_bgcolor(const uint32_t color888);
size_t tft_buffer_size();
void tft_lock_buffer(bool b);
int tft_get_mode(void);
bool tft_is_mode_text(int mode);
void tft_set_cursor_color(uint8_t);
int tft_get_default_mode(void);

void tft_init();

void refresh_lcd();
