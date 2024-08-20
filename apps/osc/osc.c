#include "m-os-api.h"
#include <hardware/gpio.h>

typedef struct screen {
    uint8_t* b;
    uint32_t w;
    uint32_t h;
    uint8_t bit;
} screen_t;

void draw_rect(screen_t* scr, int x, int y, int w, int h, uint8_t color) {
    if (scr->bit == 8) {
        for (int xi = x; xi < x + w; ++xi) {

        }
        return;
    }
}


inline static int shift_screen(int x, uint32_t w, uint32_t h, uint8_t bit, uint8_t* buff) {
    __memset(buff, 0xFF, (w * h * bit) >> 3); // TODO:
    x = 0;
    return x;
}

inline static void plot(int x, int y, uint8_t c, uint32_t w, uint32_t h, uint8_t bit, uint8_t* buff) {
    if (bit == 8) {
        buff[w * y + x] = c;
        return;
    }
    if (bit == 4) {
        buff[(w * y + x) >> 1] = c;
        return;
    }
    if (bit == 2) {
        buff[(w * y + x) >> 3] = c;
        return;
    }
    if (bit == 1) {
        buff[(w * y + x) >> 4] = c;
        return;
    }
}

int main(void) {
    cmd_ctx_t* ctx = get_cmd_ctx();
    if (ctx->argc != 2) {
        fprintf(ctx->std_err, "Usage: osc #\n where # - integer number of graphics mode\n");
        return 1;
    }
    int mode = atoi(ctx->argv[1]);
    if (graphics_is_mode_text(mode)) {
        fprintf(ctx->std_err, "mode #%d is marked for text output\n", mode);
        return -2;
    }
    int prev = graphics_get_mode();
    if (!graphics_set_mode(mode)) {
        fprintf(ctx->std_err, "Unsupported mode #%d\n", mode);
        return -1;
    }
 //   graphics_set_con_pos(-1, -1); // do not show cursor in graphics mode
    screen_t scr;
    scr.b = get_buffer();
    scr.w = get_screen_width();
    scr.h = get_screen_height();
    scr.bit = get_screen_bitness();
    draw_rect(&scr, 0, 0, scr.w, scr.h, 0xFF);
//    __memset(buff, 0xFF, (w * h * bit) >> 3);
    int x = 0;
    int hi_level = 10;
    int lo_level = scr.h - 10;
    while (getch_now() != CHAR_CODE_ESC) {
     //   int y = gpio_get(WAV_IN_PIO) ? hi_level : lo_level;
     //   plot(x, y, w, 0b00110000, h, bit, buff);
     //   ++x;
     //   if (x > w) {
     //       x = shift_screen(x, w, h, bit, buff);
     //   }
     //   vTaskDelay(1);
    }
    graphics_set_mode(prev);
    graphics_set_con_pos(0, 0);
    printf("Mode info: %d x %d x %d bits\n", scr.w, scr.h, scr.bit);
    return 0;
}

int __required_m_api_verion(void) {
    return M_API_VERSION;
}
