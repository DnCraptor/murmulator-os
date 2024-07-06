#ifdef __cplusplus
extern "C" {
#endif

#include "stdbool.h"
#include "stdio.h"
#include "stdint.h"
#include "ff.h"
#include "app.h"

#ifdef TFT
#include "st7789.h"
#endif
#ifdef HDMI
#include "hdmi.h"
#endif
#ifdef VGA
#include "vga.h"
#endif
#ifdef TV
#include "tv.h"
#endif

void graphics_init();

void graphics_set_buffer(uint8_t* buffer, uint16_t width, uint16_t height);

void graphics_set_offset(int x, int y);

void graphics_set_palette(uint8_t i, uint32_t color);

void graphics_set_textbuffer(uint8_t* buffer);

void graphics_set_bgcolor(uint32_t color888);

void graphics_set_flashmode(bool flash_line, bool flash_frame);

void draw_text(const char* string, int x, int y, uint8_t color, uint8_t bgcolor);
void draw_window(const char* title, uint32_t x, uint32_t y, uint32_t width, uint32_t height);
void goutf(const char *__restrict str, ...) _ATTRIBUTE ((__format__ (__printf__, 1, 2)));
void fgoutf(FIL *f, const char *__restrict str, ...) _ATTRIBUTE ((__format__ (__printf__, 2, 3)));
void __putc(char);
void gouta(char* buf);
void gbackspace();
void graphics_set_con_pos(int x, int y);
void graphics_set_con_color(uint8_t color, uint8_t bgcolor);
uint32_t get_buffer_width();
uint32_t get_buffer_height();
uint8_t* get_buffer();
size_t get_buffer_size();
uint8_t get_buffer_bitness();
void cleanup_graphics();
void cleanup_graphics_driver();

typedef void (*vv_fn)(void);
typedef struct graphics_driver {
    cmd_ctx_t* ctx;
    vv_fn init;
} graphics_driver_t;
void install_graphics_driver(graphics_driver_t*);

void clrScr(uint8_t color);
void graphics_set_mode(int mode);

extern volatile int pos_x;
extern volatile int pos_y;
extern volatile bool cursor_blink_state;


#ifdef __cplusplus
}
#endif
