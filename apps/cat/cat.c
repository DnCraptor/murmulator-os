#include "m-os-api.h"

static FIL f2;
static char buf[512];

int main() {
    cmd_startup_ctx_t* ctx = get_cmd_startup_ctx();
    if (ctx->tokens == 1) {
        fgoutf(ctx->pstderr, "Unable to type file with no name\n");
        return 1;
    }
    char* fn = (char*)ctx->cmd + (next_token(ctx->cmd_t) - ctx->cmd_t);
    if (f_open(&f2, fn, FA_READ) != FR_OK) {
        fgoutf(ctx->pstderr, "Unable to open file: '%s'\n", fn);
        return 2;
    }
    UINT rb;
    while(f_read(&f2, buf, 512, &rb) == FR_OK && rb > 0) {
        buf[rb] = 0;
        fgoutf(ctx->pstdout, buf);
    }
    f_close(&f2);
    return 0;
}
