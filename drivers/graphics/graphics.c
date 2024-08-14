#include "graphics.h"
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include <stdarg.h>

#ifndef logMsg
   
#ifdef DEBUG_VGA
char vga_dbg_msg[1024] = { 0 };
#define DBG_PRINT(...) { sprintf(vga_dbg_msg + strlen(vga_dbg_msg), __VA_ARGS__); }
#else
#define DBG_PRINT(...)
#endif

#endif

void common_set_con_pos(int x, int y);
int common_con_x(void);
int common_con_y(void);
void common_set_con_color(uint8_t color, uint8_t bgcolor);
void common_print(char* buf);
void common_backspace(void);
void common_draw_text(const char* string, int x, int y, uint8_t color, uint8_t bgcolor);
bool common_set_font(uint8_t width, uint8_t height);
uint8_t common_get_font_width(void);
uint8_t common_get_font_height(void);
bool common_set_ext_font(uint8_t*, uint8_t width, uint8_t height);
uint8_t* common_get_font_table(void);

const static graphics_driver_t internal_vga_driver = {
    0, //ctx
    vga_driver_init,
    vga_cleanup,
    vga_set_mode, // set_mode
    vga_is_text_mode, // is_text
    get_vga_console_width,
    get_vga_console_height,
    get_vga_screen_width,
    get_vga_screen_height,
    get_vga_buffer,
    set_vga_buffer,
    vga_clr_scr,
    common_draw_text,
    get_vga_buffer_bitness,
    get_vga_buffer_bitness,
    0, // set_offsets
    vga_set_bgcolor,
    vga_buffer_size,
    common_set_con_pos,
    common_con_x,
    common_con_y,
    common_set_con_color,
    common_print,
    common_backspace,
    vga_lock_buffer,
    vga_get_mode,
    vga_is_mode_text,
    vga_set_cursor_color,
    vga_get_default_mode,
    common_set_font,
    common_get_font_width,
    common_get_font_height,
    common_set_ext_font,
    common_get_font_table
};

#ifdef HDMI
const static graphics_driver_t internal_hdmi_driver = {
    0, //ctx
    hdmi_driver_init,
    hdmi_cleanup,
    hdmi_set_mode, // set_mode
    hdmi_is_text_mode, // is_text
    hdmi_console_width,
    hdmi_console_height,
    hdmi_screen_width,
    hdmi_screen_height,
    get_hdmi_buffer,
    set_hdmi_buffer,
    hdmi_clr_scr,
    common_draw_text,
    get_hdmi_buffer_bitness,
    get_hdmi_buffer_bitness,
    hdmi_set_offset, // set_offsets
    hdmi_set_bgcolor,
    hdmi_buffer_size,
    common_set_con_pos,
    common_con_x,
    common_con_y,
    common_set_con_color,
    common_print,
    common_backspace,
    hdmi_lock_buffer,
    hdmi_get_mode,
    hdmi_is_mode_text,
    hdmi_set_cursor_color,
    hdmi_get_default_mode,
    common_set_font,
    common_get_font_width,
    common_get_font_height,
    common_set_ext_font,
    common_get_font_table
};
#endif

#ifdef TV
const static graphics_driver_t internal_tv_driver = {
    0, //ctx
    tv_driver_init,
    tv_cleanup,
    tv_set_mode, // set_mode
    tv_is_text_mode, // is_text
    tv_console_width,
    tv_console_height,
    tv_console_width,
    tv_console_height,
    get_tv_buffer,
    set_tv_buffer,
    tv_clr_scr,
    common_draw_text,
    get_tv_buffer_bitness,
    get_tv_buffer_bitness,
    tv_set_offset, // set_offsets
    tv_set_bgcolor,
    tv_buffer_size,
    common_set_con_pos,
    common_con_x,
    common_con_y,
    common_set_con_color,
    common_print,
    common_backspace,
    tv_lock_buffer,
    tv_get_mode,
    tv_is_mode_text,
    tv_set_cursor_color,
    tv_get_default_mode,
    common_set_font,
    common_get_font_width,
    common_get_font_height,
    common_set_ext_font,
    common_get_font_table
};
#endif

#ifdef SOFTTV
const static graphics_driver_t internal_stv_driver = {
    0, //ctx
    stv_driver_init,
    0, // stv_cleanup,
    stv_set_mode, // set_mode
    stv_is_text_mode, // is_text
    stv_console_width,
    stv_console_height,
    stv_console_width,
    stv_console_height,
    get_stv_buffer,
    set_stv_buffer,
    stv_clr_scr,
    common_draw_text,
    get_stv_buffer_bitness,
    get_stv_buffer_bitness,
    stv_set_offset, // set_offsets
    stv_set_bgcolor,
    stv_buffer_size,
    common_set_con_pos,
    common_con_x,
    common_con_y,
    common_set_con_color,
    common_print,
    common_backspace,
    stv_lock_buffer,
    stv_get_mode,
    stv_is_mode_text,
    stv_set_cursor_color,
    stv_get_default_mode,
    common_set_font,
    common_get_font_width,
    common_get_font_height,
    common_set_ext_font,
    common_get_font_table
};
#endif

static volatile graphics_driver_t* __scratch_y("_driver_bss") graphics_driver = 0;

int graphics_get_default_mode(void) {
    if (graphics_driver != 0  && graphics_driver->get_default_mode) {
        return graphics_driver->get_default_mode();
    }
    return 0;
}

uint8_t* graphics_get_font_table(void) {
    if (graphics_driver != 0  && graphics_driver->get_font_table) {
        return graphics_driver->get_font_table();
    }
    return 0;
}
uint8_t graphics_get_font_width(void) {
    if (graphics_driver != 0  && graphics_driver->get_font_width) {
        return graphics_driver->get_font_width();
    }
    return 0;
}
uint8_t graphics_get_font_height(void) {
    if (graphics_driver != 0  && graphics_driver->get_font_height) {
        return graphics_driver->get_font_height();
    }
    return 0;
}
bool graphics_set_font(uint8_t w, uint8_t h) {
    if (graphics_driver != 0  && graphics_driver->set_font) {
        return graphics_driver->set_font(w, h);
    }
    return false;
}
bool graphics_set_ext_font(uint8_t* t, uint8_t w, uint8_t h) {
    if (graphics_driver != 0  && graphics_driver->set_ext_font) {
        return graphics_driver->set_ext_font(t, w, h);
    }
    return false;
}


void graphics_init(int drv_type) {
    if (graphics_driver == 0) {
        switch(drv_type) {
#ifdef HDMI
            case HDMI_DRV:
                graphics_driver = &internal_hdmi_driver;
                break;
#endif
#ifdef TV
            case RGB_DRV:
                graphics_driver = &internal_tv_driver;
                break;
#endif
#ifdef SOFTTV
            case SOFTTV_DRV:
                graphics_driver = &internal_stv_driver;
                break;
#endif
            default:
                graphics_driver = &internal_vga_driver;
                break;
        }
    }
    DBG_PRINT("graphics_init %ph\n", graphics_driver);
    if(graphics_driver && graphics_driver->init) {
        DBG_PRINT("graphics_init->init %ph\n", graphics_driver->init);
        graphics_driver->init();
    }
    DBG_PRINT("graphics_init %ph exit\n", graphics_driver);
    switch(drv_type) {
#ifdef HDMI
        case HDMI_DRV:
            hdmi_init();
            break;
#endif
#ifdef TV
        case RGB_DRV:
            tv_init();
            break;
#endif
#ifdef SOFTTV
        case SOFTTV_DRV:
            stv_init();
            break;
#endif
        default:
            vga_init();
            break;
    }
}

void set_cursor_color(uint8_t color) {
    if(graphics_driver && graphics_driver->set_cursor_color) {
        return graphics_driver->set_cursor_color(color);
    }
}

bool is_buffer_text(void) { // TODO: separate calls by supported or not
    if(graphics_driver && graphics_driver->is_text) {
        return graphics_driver->is_text();
    }
    return true;
}

uint32_t get_console_width() {
    if(graphics_driver && graphics_driver->console_width) {
        return graphics_driver->console_width();
    }
    return 0;
}
uint32_t get_console_height() {
    if(graphics_driver && graphics_driver->console_height) {
        return graphics_driver->console_height();
    }
    return 0;
}
uint32_t get_screen_width() {
    if(graphics_driver && graphics_driver->screen_width) {
        return graphics_driver->screen_width();
    }
    return 0;
}
uint32_t get_screen_height() {
    if(graphics_driver && graphics_driver->screen_height) {
        return graphics_driver->screen_height();
    }
    return 0;
}
uint8_t* get_buffer() {
    if(graphics_driver && graphics_driver->buffer) {
        return graphics_driver->buffer();
    }
    return 0;
}
void graphics_set_buffer(uint8_t* buffer) {
    if(graphics_driver && graphics_driver->set_buffer) {
        graphics_driver->set_buffer(buffer);
    }
}

void clrScr(const uint8_t color) {
    if(graphics_driver && graphics_driver->cls) {
        graphics_driver->cls(color);
    }
}

void draw_text(const char* string, int x, int y, uint8_t color, uint8_t bgcolor) {
    if(graphics_driver && graphics_driver->draw_text) {
        graphics_driver->draw_text(string, x, y, color, bgcolor);
    }
}

void draw_window(const char* title, uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
    char line[width + 1];
    memset(line, 0, sizeof line);
    width--;
    height--;
    // Рисуем рамки
    memset(line, 0xCD, width); // ═══
    line[0] = 0xC9; // ╔
    line[width] = 0xBB; // ╗
    draw_text(line, x, y, 11, 1);
    line[0] = 0xC8; // ╚
    line[width] = 0xBC; //  ╝
    draw_text(line, x, height + y, 11, 1);
    memset(line, ' ', width);
    line[0] = line[width] = 0xBA;
    for (int i = 1; i < height; i++) {
        draw_text(line, x, y + i, 11, 1);
    }
    snprintf(line, width - 1, " %s ", title);
    draw_text(line, x + (width - strlen(line)) / 2, y, 14, 3);
}

void __putc(char c) {
    char t[2] = { c, 0 };
    gouta(t);
}

void goutf(const char *__restrict str, ...) {
    FIL* f = get_stdout();
    va_list ap;
//#if DEBUG_HEAP_SIZE
    char buf[512];
//#else
//    char* buf = (char*)pvPortMalloc(512);
//#endif
    va_start(ap, str);
    vsnprintf(buf, 512, str, ap); // TODO: optimise (skip)
    va_end(ap);
    if (!f) {
        gouta(buf);
    } else {
        UINT bw;
        f_write(f, buf, strlen(buf), &bw); // TODO: error handling
    }
//#if !DEBUG_HEAP_SIZE
//    vPortFree(buf);
//#endif
}

void fgoutf(FIL *f, const char *__restrict str, ...) {
//    char* buf = (char*)pvPortMalloc(512);
    char buf[512];
    va_list ap;
    va_start(ap, str);
    vsnprintf(buf, 512, str, ap); // TODO: optimise (skip)
    va_end(ap);
    if (!f) {
        gouta(buf);
    } else {
        UINT bw;
        f_write(f, buf, strlen(buf), &bw); // TODO: error handling
    }
//    vPortFree(buf);
}

graphics_driver_t* get_graphics_driver() {
    DBG_PRINT("get_graphics_driver %ph\n", graphics_driver);
    return graphics_driver;
}

void install_graphics_driver(graphics_driver_t* gd) {
    DBG_PRINT("install_graphics_driver %ph\n", gd);
    if (graphics_driver) {
        DBG_PRINT("install_graphics_driver: to cleanup %ph\n", graphics_driver);
        cleanup_graphics();
    }
    graphics_driver = gd;
    DBG_PRINT("install_graphics_driver to init %ph\n", gd);
    graphics_init(VGA_DRV); // TODO: detect type and reject!!
    DBG_PRINT("install_graphics_driver exit\n");
}

bool graphics_set_mode(int mode) {
    if(graphics_driver && graphics_driver->set_mode) {
        return graphics_driver->set_mode(mode);
    }
    return false;
}
bool graphics_is_mode_text(int mode) {
    if(graphics_driver && graphics_driver->is_mode_text) {
        return graphics_driver->is_mode_text(mode);
    }
    return false;
}

int graphics_get_mode(void) {
    if(graphics_driver && graphics_driver->get_mode) {
        return graphics_driver->get_mode();
    }
    return 0;
}

void cleanup_graphics(void) {
    if(graphics_driver && graphics_driver->cleanup) {
        graphics_driver->cleanup();
    }
}

uint8_t get_console_bitness() {
    if(graphics_driver && graphics_driver->console_bitness) {
        return graphics_driver->console_bitness();
    }
    return 0;
}

uint8_t get_screen_bitness() {
    if(graphics_driver && graphics_driver->screen_bitness) {
        return graphics_driver->screen_bitness();
    }
    return 0;
}

void graphics_set_offset(const int x, const int y) {
    if(graphics_driver && graphics_driver->set_offsets) {
        graphics_driver->set_offsets(x, y);
    }
}

void graphics_set_bgcolor(const uint32_t color888) {
    if(graphics_driver && graphics_driver->set_bgcolor) {
        graphics_driver->set_bgcolor(color888);
    }
}
void graphics_lock_buffer(bool b) {
    if(graphics_driver && graphics_driver->lock_buffer) {
        graphics_driver->lock_buffer(b);
    }
}

size_t get_buffer_size() {
    if(graphics_driver && graphics_driver->allocated) {
        return graphics_driver->allocated();
    }
    return 0;
}

void graphics_set_con_pos(int x, int y) {
    if(graphics_driver && graphics_driver->set_con_pos) {
        graphics_driver->set_con_pos(x, y);
    }
}

int graphics_con_x(void) {
    if(graphics_driver && graphics_driver->pos_x) {
        return graphics_driver->pos_x();
    }
    return 0;
}
int graphics_con_y(void) {
    if(graphics_driver && graphics_driver->pos_y) {
        return graphics_driver->pos_y();
    }
    return 0;
}

void graphics_set_con_color(uint8_t color, uint8_t bgcolor) {
    if(graphics_driver && graphics_driver->set_con_color) {
        graphics_driver->set_con_color(color, bgcolor);
    }
}

void gouta(char* buf) {
    if(graphics_driver && graphics_driver->print) {
        graphics_driver->print(buf);
    }
}

void gbackspace() {
    if(graphics_driver && graphics_driver->backspace) {
        graphics_driver->backspace();
    }
}

// common
#include "font6x8.h"
#include "fnt8x16.h"

volatile int __scratch_y("_driver_text") pos_x = 0;
volatile int __scratch_y("_driver_text") pos_y = 0;
volatile uint8_t __scratch_y("_driver_text") con_color = 7;
volatile uint8_t __scratch_y("_driver_text") con_bgcolor = 0;
volatile uint8_t __scratch_y("_driver_text") font_width = 8;
volatile uint8_t __scratch_y("_driver_text") font_height = 16;
volatile uint8_t* __scratch_y("_driver_text") font_table = font_8x16;

uint8_t* common_get_font_table(void) {
    return font_table;
}

bool common_set_ext_font(uint8_t* table, uint8_t width, uint8_t height) {
    if (font_height == 8  || font_height == 6) { // unsupported for now heights
        return false;
    }
    font_width = width;
    font_height = height;
    font_table = table;
    return true;
}

bool common_set_font(uint8_t width, uint8_t height) {
    if (width == 8 && height == 16) {
        font_width = width;
        font_height = height;
        font_table = font_8x16;
        return true;
    }
    if (width == 6 && height == 8) {
        font_width = width;
        font_height = height;
        font_table = font_6x8;
        return true;
    }
    return false;
}
uint8_t common_get_font_width(void) {
    return font_width;
}
uint8_t common_get_font_height(void) {
    return font_height;
}

void common_set_con_pos(int x, int y) {
    pos_x = x;
    pos_y = y;
}

int common_con_x(void) {
    return pos_x;
}
int common_con_y(void) {
    return pos_y;
}

void common_set_con_color(uint8_t color, uint8_t bgcolor) {
    con_color = color;
    con_bgcolor = bgcolor;
}

static char* common_rollup(char* t_buf, uint32_t width) {
    char* b = get_buffer();
    uint32_t height = get_console_height();
    if (pos_y >= height - 1) {
        memmove(b, b + width * 2, width * (height - 2) * 2);
        t_buf = b + width * (height - 2) * 2;
        for(int i = 0; i < width; ++i) {
            *t_buf++ = ' ';
            *t_buf++ = con_bgcolor << 4 | con_color & 0xF;
        }
        pos_y = height - 2;
    }
    return b + width * 2 * pos_y + 2 * pos_x;
}

extern uint16_t txt_palette[16];

static void common_print_char(uint8_t* graphics_buffer, uint32_t width, uint32_t x, uint32_t y, uint8_t color, uint8_t bgcolor, uint16_t c) {
    uint8_t bit = get_screen_bitness();
    if (bit == 8) {
        uint8_t* p0 = graphics_buffer + width * y * font_height + x * font_width;

        uint8_t b = (color & 1) ? ((color >> 3) ? 3 : 2) : 0;
        uint8_t r = (color & 4) ? ((color >> 3) ? 3 : 2) : 0;
        uint8_t g = (color & 2) ? ((color >> 3) ? 3 : 2) : 0;
        uint8_t cf = (((r << 4) | (g << 2) | b) & 0x3f) | 0xc0;
        b = (bgcolor & 1) ? ((bgcolor >> 3) ? 3 : 2) : 0;
        r = (bgcolor & 4) ? ((bgcolor >> 3) ? 3 : 2) : 0;
        g = (bgcolor & 2) ? ((bgcolor >> 3) ? 3 : 2) : 0;
        uint8_t cb = (((r << 4) | (g << 2) | b) & 0x3f) | 0xc0;

        for (int glyph_line = 0; glyph_line < font_height; ++glyph_line) {
            uint8_t* p = p0 + width * glyph_line;
            uint8_t glyph_pixels = font_table[c * font_height + glyph_line];
            //con_bgcolor << 4 | con_color & 0xF
            *p++ =  (glyph_pixels & 1)       ? cf : cb;
            *p++ = ((glyph_pixels >> 1) & 1) ? cf : cb;
            *p++ = ((glyph_pixels >> 2) & 1) ? cf : cb;
            *p++ = ((glyph_pixels >> 3) & 1) ? cf : cb;
            *p++ = ((glyph_pixels >> 4) & 1) ? cf : cb;
            *p++ = ((glyph_pixels >> 5) & 1) ? cf : cb;
            *p++ = ((glyph_pixels >> 6) & 1) ? cf : cb;
            *p   =  (glyph_pixels >> 7)      ? cf : cb;
        }
        return;
    }
    if (bit == 4) {
        uint8_t* p0 = graphics_buffer + (width * y * font_height >> 1) + (x * font_width >> 1);
        uint8_t cf = color & 0x0F;
        uint8_t cb = bgcolor & 0x0F;
        for (int glyph_line = 0; glyph_line < font_height; ++glyph_line) {
            uint8_t* p = p0 + (width * glyph_line >> 1);
            uint8_t glyph_pixels = font_table[c * font_height + glyph_line];
            *p++ = (  (glyph_pixels & 1)       ? cf : cb ) | (( ((glyph_pixels >> 1) & 1) ? cf : cb ) << 4);
            *p++ = ( ((glyph_pixels >> 2) & 1) ? cf : cb ) | (( ((glyph_pixels >> 3) & 1) ? cf : cb ) << 4);
            *p++ = ( ((glyph_pixels >> 4) & 1) ? cf : cb ) | (( ((glyph_pixels >> 5) & 1) ? cf : cb ) << 4);
            *p   = ( ((glyph_pixels >> 6) & 1) ? cf : cb ) | ((  (glyph_pixels >> 7)      ? cf : cb ) << 4);
        }
        return;
    }
}

static void graphics_rollup(uint8_t* graphics_buffer, uint32_t width) {
    uint32_t height = get_screen_height();
    uint8_t bit = get_screen_bitness();
    uint32_t h = height / font_height;
    if (pos_y >= h - 1) {
        uint32_t sz = (width * (height - 2 * font_height) * bit) >> 3;
        memmove(graphics_buffer, graphics_buffer + (width * bit * font_height >> 3), sz);
        memset(graphics_buffer + sz, 0, width * bit * font_height >> 3);
        pos_y = h - 2;
    }
}

void common_print(char* buf) {
    uint8_t* graphics_buffer = get_buffer();
    if (!graphics_buffer) {
        return;
    }
    uint32_t width = get_screen_width();
    char c;
    if (!is_buffer_text()) {
        while (c = *buf++) {
            if (c == '\r') continue; // ignore DOS stile \r\n, only \n to start new line
            if (c == '\n') {
                pos_x = 0;
                pos_y++;
                graphics_rollup(graphics_buffer, width);
                continue;
            }
            if (pos_x * font_width >= width) {
                pos_x = 0;
                pos_y++;
                graphics_rollup(graphics_buffer, width);
                common_print_char(graphics_buffer, width, pos_x, pos_y, con_color, con_bgcolor, c);
            } else {
                common_print_char(graphics_buffer, width, pos_x, pos_y, con_color, con_bgcolor, c);
            }
            pos_x++;
        }
        return;
    }
    uint8_t* t_buf = graphics_buffer + width * 2 * pos_y + 2 * pos_x;
    while (c = *buf++) {
        if (c == '\r') continue; // ignore DOS stile \r\n, only \n to start new line
        if (c == '\n') {
            pos_x = 0;
            pos_y++;
            t_buf = common_rollup(t_buf, width);
            continue;
        }
        pos_x++;
        if (pos_x >= width) {
            pos_x = 0;
            pos_y++;
            t_buf = common_rollup(t_buf, width);
            *t_buf++ = c;
            *t_buf++ = con_bgcolor << 4 | con_color & 0xF;
            pos_x++;
        } else {
            *t_buf++ = c;
            *t_buf++ = con_bgcolor << 4 | con_color & 0xF;
        }
    }
}

void common_backspace(void) {
    char* graphics_buffer = get_buffer();
    if (!graphics_buffer) return;
    uint32_t width = get_screen_width();
    if (!is_buffer_text()) {
     //   common_print_char(graphics_buffer, width, pos_x, pos_y, ' ');
        pos_x--;
        if (pos_x < 0) {
            pos_x = width / font_width;
            common_print_char(graphics_buffer, width, pos_x, pos_y, con_color, con_bgcolor, ' ');
            --pos_y;
            --pos_x;
            if (pos_y < 0) {
                pos_y = 0;
            }
        }
        common_print_char(graphics_buffer, width, pos_x, pos_y, con_color, con_bgcolor, ' ');
        return;
    }
    pos_x--;
    if (pos_x < 0) {
        pos_x = width - 2;
        pos_y--;
        if (pos_y < 0) {
            pos_y = 0;
        }
    }
    char* t_buf = graphics_buffer + width * 2 * pos_y + 2 * pos_x;
    *t_buf++ = ' ';
    *t_buf++ = con_bgcolor << 4 | con_color & 0xF;
}

void common_draw_text(const char* string, int x, int y, uint8_t color, uint8_t bgcolor) {
    char* graphics_buffer = get_buffer();
    if (!graphics_buffer) return;
    uint32_t width = get_screen_width();
    if (!is_buffer_text()) {
        for (int xi = x; xi < width * 2; ++xi) {
            if (!(*string)) break;
            common_print_char(graphics_buffer, width, xi, y, color, bgcolor, *string++);
        }
        return;
    }
    uint8_t* t_buf = graphics_buffer + width * 2 * y + 2 * x;
    uint8_t c = (bgcolor << 4) | (color & 0xF);
    for (int xi = x; xi < width * 2; ++xi) {
        if (!(*string)) break;
        *t_buf++ = *string++;
        *t_buf++ = c;
    }
}

// TODO: Сделать настраиваемо
const uint8_t textmode_palette[16] = {
    200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215
};
