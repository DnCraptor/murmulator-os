#include "m-os-api.h"

int main() {
    cmd_ctx_t* ctx = get_cmd_ctx();
    if (ctx->argc == 0) {
        fgoutf(ctx->std_err, "ATTTENTION! BROKEN EXECUTION CONTEXT [%p]!\n", ctx);
        return -1;
    }
    if (ctx->argc != 1 || !ctx->std_in || ctx->next) {
        fgoutf(ctx->std_err, "Pls. use '%s' for pipe end only\n", ctx->argv[0]);
        return 1;
    }
    uint32_t width = get_console_width();
    uint32_t height = get_console_height();
    uint32_t lines = 0;
    uint32_t cols = 0;
    while(1) {
        int ic = f_getc(ctx->std_in);
        // printf("[%d]:\n", ic);
        if (ic == -1 /*EOF*/) {
            break;
        }
        char c = ic & 0xFF;
        if (c == '\n') {
            lines++;
            // printf("lines: %d\n", lines);
            cols = 0;
        } else {
            cols++;
        }
        if (cols > width) {
            cols = 1;
            lines++;
            // printf("lines: %d\n", lines);
        }
        if (c == '\r') {
            continue;
        }
        // printf("wait c\n");
        putc(c);
        if (height - 3 <= lines) {
            int icc = getc(NULL);
            if (icc == -1 /*Ctrl+C*/) break;
            char cc = icc & 0xFF;
            if (cc == 0x1B /*ESC*/) {
                break;
            } else if (cc == 18 /*down*/ || cc == ' ' || cc == '\n') {
                lines = 0;
                continue;
            }
        }
    }
    return 0;
}

int __required_m_api_verion(void) {
    return M_API_VERSION;
}
