#include "m-os-api.h"

int main(void) {
    cmd_ctx_t* ctx = get_cmd_ctx();
    if (ctx->argc != 2) {
        fprintf(ctx->std_err, "Usage: mode #\n where # - integer number\n");
        return 1;
    }
    int mode = atoi(ctx->argv[1]);
    if (!graphics_is_mode_text(mode)) {
        fprintf(ctx->std_err, "mode #%d is not ready for text output\n", mode);
        return -2;
    }
    if (!graphics_set_mode(mode)) {
        fprintf(ctx->std_err, "Unsupported mode #%d\n", mode);
        return -1;
    }
    return 0;
}

int __required_m_api_verion(void) {
    return M_API_VERSION;
}
