#include "m-os-api.h"

FILINFO fi;
int main() {
    cmd_startup_ctx_t* ctx = get_cmd_startup_ctx();
    if (ctx->tokens == 1) {
        fgoutf(ctx->pstderr, "Unable to remove nothing\n");
        return 1;
    } else if (ctx->tokens == 2) {
        char *f = (char*)ctx->cmd + (next_token(ctx->cmd_t) - ctx->cmd_t);
        if (f_stat(f, &fi) != FR_OK) {
            fgoutf(ctx->pstderr, "File '%s' not found\n", f);
            return 2;
        }
        if (fi.fattrib & AM_DIR) {
            fgoutf(ctx->pstderr, "WARN: '%s' is a folder\n", f);
        }
        if (f_unlink(f) != FR_OK) {
            fgoutf(ctx->pstderr, "Unable to unlink: '%s'\n", f);
            return 4;
        }
        fgoutf(ctx->pstdout, "Unlink '%s' passed\n", f);
    } else {
        fgoutf(ctx->pstderr, "Unable to remove more than one file\n");
        return 5;
    }
    return 0;
}

int __required_m_api_verion() {
    return 2;
}
