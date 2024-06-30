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
    uint32_t width = get_text_buffer_width();
    uint32_t height = get_text_buffer_height();
    uint32_t lines = 0;
    uint32_t cols = 0;
    while(1) {
        int ic = getc(ctx->std_in); // TODO: ctx->std_in
        if (ic == -1 /*EOF*/) {
            vTaskDelay(50);
            continue;
        }
        char c = ic & 0xFF;
        if (c == '\n') {
            lines++;
            cols = 0;
        } else {
            cols++;
        }
        if (cols > width) {
            cols = 1;
            lines++;
        }
        if (c == '\r') {
            continue;
        }
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
