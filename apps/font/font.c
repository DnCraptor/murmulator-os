#include "m-os-api.h"

int main(void) {
    cmd_ctx_t* ctx = get_cmd_ctx();
    if (ctx->argc == 0) {
        fprintf(ctx->std_err, "ATTTENTION! BROKEN EXECUTION CONTEXT [%p]!\n", ctx);
        return -1;
    }
    if (ctx->argc != 1 && ctx->argc != 3) {
        fprintf(ctx->std_err,
            "Unexpected number of argument: %d\n"
            "Use: font [width] [height]\n", ctx->argc
        );
        return 3;
    }
    if (ctx->argc == 3) {
        int w = atoi(ctx->argv[1]);
        int h = atoi(ctx->argv[2]);
        if (!graphics_set_font(w, h)) {
            fprintf(ctx->std_err, "Usupported font dimentions: [%s]x[%s]\n", ctx->argv[1], ctx->argv[2]);
        }
    }
    printf("Current font [%p] [%d]x[%d]\n", graphics_get_font_table(), graphics_get_font_width(), graphics_get_font_height());
    return 0;
}

