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

static graphics_driver_t internal_vga_driver = {
    0, //ctx
    vga_driver_init,
    vga_cleanup,
    vga_set_mode, // set_mode
    vga_is_text_mode, // is_text
    get_vga_console_width,
    get_vga_console_height,
    get_vga_console_width,
    get_vga_console_height,
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
    vga_set_cursor_color
};

static graphics_driver_t internal_hdmi_driver = {
    0, //ctx
    hdmi_driver_init,
    hdmi_cleanup,
    hdmi_set_mode, // set_mode
    hdmi_is_text_mode, // is_text
    hdmi_console_width,
    hdmi_console_height,
    hdmi_console_width,
    hdmi_console_height,
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
    hdmi_set_cursor_color
};

#ifdef TV
static graphics_driver_t internal_tv_driver = {
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
    tv_set_cursor_color
};
#endif

static volatile graphics_driver_t* graphics_driver = 0;

void graphics_init(int drv_type) {
    if (graphics_driver == 0) {
        switch(drv_type) {
            case HDMI_DRV:
                graphics_driver = &internal_hdmi_driver;
                break;
#ifdef TV
            case TV_DRV:
                graphics_driver = &internal_tv_driver;
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
        case HDMI_DRV:
            hdmi_init();
            break;
#ifdef TV
        case TV_DRV:
            tv_init();
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
volatile int __scratch_y("_driver_text") pos_x = 0;
volatile int __scratch_y("_driver_text") pos_y = 0;
volatile uint8_t __scratch_y("_driver_text") con_color = 7;
volatile uint8_t __scratch_y("_driver_text") con_bgcolor = 0;

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

static char* common_rollup(char* t_buf) {
    char* b = get_buffer();
    uint32_t width = get_console_width();
    uint32_t height = get_console_height();
    if (pos_y >= height - 1) {
        memcpy(b, b + width * 2, width * (height - 2) * 2);
        t_buf = b + width * (height - 2) * 2;
        for(int i = 0; i < width; ++i) {
            *t_buf++ = ' ';
            *t_buf++ = con_bgcolor << 4 | con_color & 0xF;
        }
        pos_y = height - 2;
    }
    return b + width * 2 * pos_y + 2 * pos_x;
}

void common_print(char* buf) {
    char* graphics_buffer = get_buffer();
    if (!graphics_buffer) return;
    uint32_t width = get_console_width();
    uint8_t* t_buf = graphics_buffer + width * 2 * pos_y + 2 * pos_x;
    char c;
    while (c = *buf++) {
        if (c == '\r') continue; // ignore DOS stile \r\n, only \n to start new line
        if (c == '\n') {
            pos_x = 0;
            pos_y++;
            t_buf = common_rollup(t_buf);
            continue;
        }
        pos_x++;
        if (pos_x >= width) {
            pos_x = 0;
            pos_y++;
            t_buf = common_rollup(t_buf);
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
    uint32_t width = get_console_width();
    uint8_t* t_buf;
    pos_x--;
    if (pos_x < 0) {
        pos_x = width - 2;
        pos_y--;
        if (pos_y < 0) {
            pos_y = 0;
        }
    }
    t_buf = graphics_buffer + width * 2 * pos_y + 2 * pos_x;
    *t_buf++ = ' ';
    *t_buf++ = con_bgcolor << 4 | con_color & 0xF;
}

void common_draw_text(const char* string, int x, int y, uint8_t color, uint8_t bgcolor) {
    char* graphics_buffer = get_buffer();
    if (!graphics_buffer) return;
    uint32_t width = get_console_width();
    uint8_t* t_buf = graphics_buffer + width * 2 * y + 2 * x;
    uint8_t c = (bgcolor << 4) | (color & 0xF);
    for (int xi = x; xi < width * 2; ++xi) {
        if (!(*string)) break;
        *t_buf++ = *string++;
        *t_buf++ = c;
    }
}
