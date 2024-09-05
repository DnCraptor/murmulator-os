#include "m-os-api.h"

int main(void) {
    cmd_ctx_t* ctx = get_cmd_ctx();
    if (ctx->argc == 1) {
        int mode = graphics_get_mode();
        printf("  Current mode: %d\n", mode);
        bool b = graphics_is_mode_text(mode);
        printf("     text only: %s\n", b ? "true" : "false");
        printf("    resolution: %dx%dx%d-bit\n", get_screen_width(), get_screen_height(), get_screen_bitness());
        if (!b) {
            printf("txt resolution: %dx%d\n", get_console_width(), get_console_height());
        }
        return 0;
    }
    if (ctx->argc > 2) {
        fprintf(ctx->std_err, "Usage: mode #\n where # - integer number\n");
        return 1;
    }
    int mode = atoi(ctx->argv[1]);
    if (!graphics_set_mode(mode)) {
        fprintf(ctx->std_err, "Unsupported mode #%d\n", mode);
        return -1;
    }
    return 0;
}
