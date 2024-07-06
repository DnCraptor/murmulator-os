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
void graphics_set_buffer(uint8_t* buffer);
void graphics_set_offset(int x, int y);
void graphics_set_bgcolor(uint32_t color888);

void draw_text(const char* string, int x, int y, uint8_t color, uint8_t bgcolor);
void draw_window(const char* title, uint32_t x, uint32_t y, uint32_t width, uint32_t height);
void goutf(const char *__restrict str, ...) _ATTRIBUTE ((__format__ (__printf__, 1, 2)));
void fgoutf(FIL *f, const char *__restrict str, ...) _ATTRIBUTE ((__format__ (__printf__, 2, 3)));
void __putc(char);
void gouta(char* buf);
void gbackspace();
void graphics_set_con_pos(int x, int y);
void graphics_set_con_color(uint8_t color, uint8_t bgcolor);
uint32_t get_console_width();
uint32_t get_console_height();
uint8_t get_console_bitness();
uint32_t get_screen_width();
uint32_t get_screen_height();
uint8_t* get_buffer();
size_t get_buffer_size();
uint8_t get_screen_bitness();
void cleanup_graphics();
bool is_buffer_text();

typedef void (*vv_fn)(void);
typedef void (*vi_fn)(int);
typedef bool (*bv_fn)(void);
typedef uint32_t (*u32v_fn)(void);
typedef uint8_t* (*pu8v_fn)(void);
typedef uint8_t (*u8v_fn)(void);
typedef void (*vpu8_fn)(uint8_t*);
typedef void (*vu8_fn)(uint8_t);
typedef void (*dt_fn)(const char* string, int x, int y, uint8_t color, uint8_t bgcolor);
typedef void (*set_offsets_fn)(const int x, const int y);
typedef void (*vcu32_fn)(const uint32_t color888);
typedef struct graphics_driver {
    cmd_ctx_t* ctx;
    vv_fn init;
    vv_fn cleanup;
    vi_fn set_mode;
    bv_fn is_text;
    u32v_fn console_width;
    u32v_fn console_height;
    u32v_fn screen_width;
    u32v_fn screen_height;
    pu8v_fn buffer;
    vpu8_fn set_buffer;
    vu8_fn cls;
    dt_fn draw_text;
    u8v_fn console_bitness;
    u8v_fn screen_bitness;
    set_offsets_fn set_offsets;
    vcu32_fn set_bgcolor;
    u32v_fn allocated;
} graphics_driver_t;
void install_graphics_driver(graphics_driver_t*);
graphics_driver_t* get_graphics_driver();
void clrScr(uint8_t color);
void graphics_set_mode(int mode);

extern volatile int pos_x;
extern volatile int pos_y;
extern volatile bool cursor_blink_state;


#ifdef __cplusplus
}
#endif
