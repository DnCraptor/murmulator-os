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
    char c;
    while(1) {
        c = getc(); // TODO: ctx->std_in
        if (c == 0x1B /*ESC*/) {
            break;
        } else if (c == 18 /*down*/) {

        }
    }
    return 0;
}

int __required_m_api_verion(void) {
    return M_API_VERSION;
}
