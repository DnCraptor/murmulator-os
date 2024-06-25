#include "m-os-api.h"

int main(void) {
    cmd_ctx_t* ctx = get_cmd_ctx();
    if (ctx->argc == 1) {
        fgoutf(ctx->std_err, "Unable to remove nothing\n");
        return 1;
    } else if (ctx->argc == 2) {
        FILINFO* pfi = malloc(sizeof(FILINFO));
        if (f_stat(ctx->argv[1], pfi) != FR_OK) {
            fgoutf(ctx->std_err, "File '%s' not found\n", ctx->argv[1]);
            free(pfi);
            return 2;
        }
        if (pfi->fattrib & AM_DIR) {
            fgoutf(ctx->std_err, "WARN: '%s' is a folder\n", ctx->argv[1]);
        }
        if (f_unlink(ctx->argv[1]) != FR_OK) {
            fgoutf(ctx->std_err, "Unable to unlink: '%s'\n", ctx->argv[1]);
            free(pfi);
            return 4;
        }
        free(pfi);
        fgoutf(ctx->std_out, "Unlink '%s' passed\n", ctx->argv[1]);
    } else {
        fgoutf(ctx->std_err, "Unable to remove more than one file\n");
        return 5;
    }
    return 0;
}

int __required_m_api_verion(void) {
    return M_API_VERSION;
}
