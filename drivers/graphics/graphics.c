#include "graphics.h"
#include <string.h>

void draw_text(const char string[TEXTMODE_COLS + 1], uint32_t x, uint32_t y, uint8_t color, uint8_t bgcolor) {
    uint8_t* t_buf = text_buffer + TEXTMODE_COLS * 2 * y + 2 * x;
    for (int xi = TEXTMODE_COLS * 2; xi--;) {
        if (!*string) break;
        *t_buf++ = *string++;
        *t_buf++ = bgcolor << 4 | color & 0xF;
    }
}

void draw_window(const char title[TEXTMODE_COLS + 1], uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
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


static int pos_x = 0;
static int pos_y = 0;
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

void goutf(const char *__restrict str, ...) {
    va_list ap;
    char buf[512]; // TODO: some const?
    va_start(ap, str);
    vsnprintf(buf, 512, str, ap); // TODO: optimise (skip)
    va_end(ap);
    uint8_t* t_buf = text_buffer + TEXTMODE_COLS * 2 * pos_y + 2 * pos_x;
    char c;
    str = buf;
    while (c = *str++) {
        if (c == '\n') {
            pos_x = 0;
            pos_y++;
            t_buf = text_buffer + TEXTMODE_COLS * 2 * pos_y + 2 * pos_x;
            continue;
        }
        pos_x++;
        if (pos_x > TEXTMODE_COLS) {
            pos_x = 0;
            pos_y++;
        }
        *t_buf++ = c;
        *t_buf++ = con_bgcolor << 4 | con_color & 0xF;
    }
}
