#include "graphics.h"
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"

static uint8_t* text_buffer = 0;
static uint text_buffer_width = 0;
static uint text_buffer_height = 0;
static graphics_driver_t internal_driver = {
    0, //ctx
    vga_init,
};
static graphics_driver_t* graphics_driver = &internal_driver;

void graphics_init() {
    if(graphics_driver && graphics_driver->init) {
        graphics_driver->init();
    }
}

uint32_t get_buffer_width() {
    return text_buffer_width;
}
uint32_t get_buffer_height() {
    return text_buffer_height;
}
uint8_t* get_buffer() {
    return text_buffer;
}

void graphics_set_textbuffer(uint8_t* buffer) {
    text_buffer = buffer;
}

void graphics_set_buffer(uint8_t* buffer, const uint16_t width, const uint16_t height) {
    text_buffer = buffer;
    text_buffer_width = width;
    text_buffer_height = height;
}

void clrScr(const uint8_t color) {
    if (!text_buffer) return;
    uint16_t* t_buf = (uint16_t *)text_buffer;
    int size = text_buffer_width * text_buffer_height;
    while (size--) *t_buf++ = color << 4 | ' ';
    graphics_set_con_pos(0, 0);
    graphics_set_con_color(7, color); // TODO:
}

void draw_text(const char* string, int x, int y, uint8_t color, uint8_t bgcolor) {
    taskENTER_CRITICAL();
    char* text_buffer = get_buffer();
    uint text_buffer_width = get_buffer_width();
    uint text_buffer_height = get_buffer_height();
    uint8_t* t_buf = text_buffer + text_buffer_width * 2 * y + 2 * x;
    uint8_t c = (bgcolor << 4) | (color & 0xF);
    for (int xi = x; xi < text_buffer_width * 2; ++xi) {
        if (!(*string)) break;
        *t_buf++ = *string++;
        *t_buf++ = c;
    }
    taskEXIT_CRITICAL();
}

void draw_window(const char* title, uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
    taskENTER_CRITICAL();
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
    taskEXIT_CRITICAL();
}


volatile int pos_x = 0;
volatile int pos_y = 0;
volatile bool cursor_blink_state = true;
static uint8_t con_color = 7;
static uint8_t con_bgcolor = 0;

void graphics_set_con_pos(int x, int y) {
    pos_x = x;
    pos_y = y;
}

void graphics_set_con_color(uint8_t color, uint8_t bgcolor) {
    con_color = color;
    con_bgcolor = bgcolor;
}

#include <stdarg.h>

char* _rollup(char* t_buf) {
    taskENTER_CRITICAL();
    char* text_buffer = get_buffer();
    uint text_buffer_width = get_buffer_width();
    uint text_buffer_height = get_buffer_height();
    if (pos_y >= text_buffer_height - 1) {
        memcpy(text_buffer, text_buffer + text_buffer_width * 2, text_buffer_width * (text_buffer_height - 2) * 2);
        t_buf = text_buffer + text_buffer_width * (text_buffer_height - 2) * 2;
        for(int i = 0; i < text_buffer_width; ++i) {
            *t_buf++ = ' ';
            *t_buf++ = con_bgcolor << 4 | con_color & 0xF;
        }
        pos_y = text_buffer_height - 2;
    }
    taskEXIT_CRITICAL();
    return text_buffer + text_buffer_width * 2 * pos_y + 2 * pos_x;
}

void gbackspace() {
    taskENTER_CRITICAL();
    char* text_buffer = get_buffer();
    uint text_buffer_width = get_buffer_width();
    uint8_t* t_buf;
    //do {
        pos_x--;
        if (pos_x < 0) {
            pos_x = text_buffer_width - 2;
            pos_y--;
            if (pos_y < 0) {
                pos_y = 0;
            }
        }
        t_buf = text_buffer + text_buffer_width * 2 * pos_y + 2 * pos_x;
    //} while(*t_buf == ' ');
    *t_buf++ = ' ';
    *t_buf++ = con_bgcolor << 4 | con_color & 0xF;
    taskEXIT_CRITICAL();
}

void __putc(char c) {
    char t[2] = { c, 0 };
    gouta(t);
}

void gouta(char* buf) {
    taskENTER_CRITICAL();
    char* text_buffer = get_buffer();
    uint text_buffer_width = get_buffer_width();
    uint8_t* t_buf = text_buffer + text_buffer_width * 2 * pos_y + 2 * pos_x;
    char c;
    while (c = *buf++) {
        if (c == '\r') continue; // ignore DOS stile \r\n, only \n to start new line
        if (c == '\n') {
            pos_x = 0;
            pos_y++;
            t_buf = _rollup(t_buf);
            continue;
        }
        pos_x++;
        if (pos_x >= text_buffer_width) {
            pos_x = 0;
            pos_y++;
            t_buf = _rollup(t_buf);
            *t_buf++ = c;
            *t_buf++ = con_bgcolor << 4 | con_color & 0xF;
            pos_x++;
        } else {
            *t_buf++ = c;
            *t_buf++ = con_bgcolor << 4 | con_color & 0xF;
        }
    }
    //char tmp[32];
    //snprintf(tmp, 32, "x:%02d y:%02d ", pos_x, pos_y);
    //draw_text(tmp, 0, 0, 7, 0);
    taskEXIT_CRITICAL();
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

void install_graphics_driver(graphics_driver_t* gd) {
    if (graphics_driver) {
        cleanup_graphics();
        if (graphics_driver != &internal_driver) {
            remove_ctx(graphics_driver->ctx);
            vPortFree(graphics_driver);
        }
    }
    graphics_driver = gd;
    graphics_init();
}

void graphics_set_mode(int mode) {

}

void cleanup_graphics() {
    cleanup_graphics_driver();
    vPortFree(text_buffer);
    text_buffer = 0;
}
