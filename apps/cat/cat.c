#include "m-os-api.h"

int main() {
    cmd_startup_ctx_t* ctx = get_cmd_startup_ctx();
    if (ctx->tokens == 1) {
        fgoutf(ctx->pstderr, "Unable to type file with no name\n");
        return 1;
    }
    if (ctx->tokens > 2) {
        fgoutf(ctx->pstderr, "Unable to type more than one file\n");
        return 1;
    }
    char* fn = (char*)ctx->cmd + (next_token(ctx->cmd_t) - ctx->cmd_t);
    FIL* f = (FIL*)pvPortMalloc(sizeof(FIL));
    if (f_open(f, fn, FA_READ) != FR_OK) {
        vPortFree(f);
        fgoutf(ctx->pstderr, "Unable to open file: '%s'\n", fn);
        return 2;
    }
    UINT rb;
    char* buf = (char*)pvPortMalloc(512);
    while(f_read(f, buf, 512, &rb) == FR_OK && rb > 0) {
        buf[rb] = 0;
        fgoutf(ctx->pstdout, buf);
    }
    vPortFree(buf);
    f_close(f);
    vPortFree(f);
    return 0;
}

int __required_m_api_verion(void) {
    return 2;
}
