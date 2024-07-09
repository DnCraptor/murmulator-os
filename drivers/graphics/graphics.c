#include "graphics.h"
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include <stdarg.h>

#ifndef logMsg
   
#ifdef DEBUG_VGA
extern char vga_dbg_msg[1024];
#define DBG_PRINT(...) { sprintf(vga_dbg_msg + strlen(vga_dbg_msg), __VA_ARGS__); }
#else
#define DBG_PRINT(...)
#endif

#endif

static graphics_driver_t internal_driver = {
    0, //ctx
    vga_init,
    vga_cleanup,
    vga_set_mode, // set_mode
    0, // is_text
    get_vga_console_width,
    get_vga_console_height,
    get_vga_console_width,
    get_vga_console_height,
    get_vga_buffer,
    set_vga_buffer,
    vga_clr_scr,
    vga_draw_text,
    get_vga_buffer_bitness,
    get_vga_buffer_bitness,
    0, // set_offsets
    vga_set_bgcolor,
    vga_buffer_size,
    vga_set_con_pos,
    vga_con_x,
    vga_con_y,
    vga_set_con_color,
    vga_print,
    vga_backspace,
    vga_lock_buffer
};
static graphics_driver_t* graphics_driver = &internal_driver;

void graphics_init() {
    DBG_PRINT("graphics_init %ph\n", graphics_driver);
    if(graphics_driver && graphics_driver->init) {
       DBG_PRINT("graphics_init->init %ph\n", graphics_driver->init);
        graphics_driver->init();
    }
    DBG_PRINT("graphics_init %ph exit\n", graphics_driver);
}

bool is_buffer_text(void) {
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
    taskENTER_CRITICAL();
    if(graphics_driver && graphics_driver->draw_text) {
        graphics_driver->draw_text(string, x, y, color, bgcolor);
    }
    taskEXIT_CRITICAL();
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
    va_list ap;
#if DEBUG_HEAP_SIZE
    char buf[512];
#else
    char* buf = (char*)pvPortMalloc(512);
#endif
    va_start(ap, str);
    vsnprintf(buf, 512, str, ap); // TODO: optimise (skip)
    va_end(ap);
    gouta(buf);
#if !DEBUG_HEAP_SIZE
    vPortFree(buf);
#endif
}

void fgoutf(FIL *f, const char *__restrict str, ...) {
    char* buf = (char*)pvPortMalloc(512);
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
    vPortFree(buf);
}

graphics_driver_t* get_graphics_driver() {
    DBG_PRINT("get_graphics_driver %ph\n", graphics_driver);
    return graphics_driver;
}

void install_graphics_driver(graphics_driver_t* gd) {
    DBG_PRINT("install_graphics_driver %ph\n", gd);
    if (graphics_driver) {
        DBG_PRINT("install_graphics_driver to cleanup %ph\n", graphics_driver);
        cleanup_graphics();
        if (graphics_driver != &internal_driver) {
            remove_ctx(graphics_driver->ctx);
            vPortFree(graphics_driver);
        }
    }
    graphics_driver = gd;
    DBG_PRINT("install_graphics_driver to init %ph\n", gd);
    graphics_init();
    DBG_PRINT("install_graphics_driver exit\n");
}

bool graphics_set_mode(int mode) {
    if(graphics_driver && graphics_driver->set_mode) {
        return graphics_driver->set_mode(mode);
    }
    return false;
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
    taskENTER_CRITICAL();
    if(graphics_driver && graphics_driver->print) {
        graphics_driver->print(buf);
    }
    taskEXIT_CRITICAL();
}

void gbackspace() {
    taskENTER_CRITICAL();
    if(graphics_driver && graphics_driver->backspace) {
        graphics_driver->backspace();
    }
    taskEXIT_CRITICAL();
}
