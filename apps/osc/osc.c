#include "m-os-api.h"
#include <hardware/gpio.h>

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
    uint32_t w = get_screen_width();
    uint32_t h = get_screen_height();
    uint8_t bit = get_screen_bitness();
    printf("Mode info: %d x %d x %d bits\n", w, h, bit);
    uint8_t* buff = get_buffer();
    while (getch_now() != CHAR_CODE_ESC) {
        bool son = gpio_get(WAV_IN_PIO);
        printf("Input signal: %d\n", son);
    }
    graphics_set_mode(prev);
    graphics_set_con_pos(0, 0);
    printf("Mode info: %d x %d x %d bits\n", w, h, bit);
    return 0;
}

int __required_m_api_verion(void) {
    return M_API_VERSION;
}
