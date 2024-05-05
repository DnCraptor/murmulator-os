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
    if (pos_y >= TEXTMODE_ROWS - 1) {
        memcpy(text_buffer, text_buffer + TEXTMODE_COLS * 2, TEXTMODE_COLS * (TEXTMODE_ROWS - 2) * 2);
        t_buf = text_buffer + TEXTMODE_COLS * (TEXTMODE_ROWS - 2) * 2;
        for(int i = 0; i < TEXTMODE_COLS; ++i) {
            *t_buf++ = ' ';
            *t_buf++ = con_bgcolor << 4 | con_color & 0xF;
        }
        pos_y = TEXTMODE_ROWS - 2;
    }
    return text_buffer + TEXTMODE_COLS * 2 * pos_y + 2 * pos_x;
}

void gbackspace() {
    uint8_t* t_buf;
    //do {
        pos_x--;
        if (pos_x < 0) {
            pos_x = TEXTMODE_COLS - 2;
            pos_y--;
            if (pos_y < 0) {
                pos_y = 0;
            }
        }
        t_buf = text_buffer + TEXTMODE_COLS * 2 * pos_y + 2 * pos_x;
    //} while(*t_buf == ' ');
    *t_buf++ = ' ';
    *t_buf++ = con_bgcolor << 4 | con_color & 0xF;
}

void gouta(char* buf) {
    uint8_t* t_buf = text_buffer + TEXTMODE_COLS * 2 * pos_y + 2 * pos_x;
    char c;
    while (c = *buf++) {
        if (c == '\n') {
            pos_x = 0;
            pos_y++;
            t_buf = _rollup(t_buf);
            continue;
        }
        pos_x++;
        if (pos_x >= TEXTMODE_COLS) {
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
}

void goutf(const char *__restrict str, ...) {
    va_list ap;
    char buf[512]; // TODO: some const?
    va_start(ap, str);
    vsnprintf(buf, 512, str, ap); // TODO: optimise (skip)
    va_end(ap);
    gouta(buf);
}

void fgoutf(FIL *f, const char *__restrict str, ...) {
    char buf[512]; // TODO: some const?
    va_list ap;
    va_start(ap, str);
    vsnprintf(buf, 512, str, ap); // TODO: optimise (skip)
    va_end(ap);
    if (!f || !f->obj.fs) {
        gouta(buf);
    } else {
        UINT bw;
        f_write(f, buf, strlen(buf), &bw); // TODO: error handling
    }
}
