#include "m-os-api.h"

static FIL fil1, fil2;
static char buf[512];

int main() {
    cmd_startup_ctx_t* ctx = get_cmd_startup_ctx();
    if (ctx->tokens < 3) {
        fgoutf(ctx->pstderr, "Usage: %s <file1> <file2>\n", ctx->cmd_t);
        return 1;
    }
    char* f1 = next_token(ctx->cmd_t);
    char* pt2 = next_token(f1);
    char* f2 = (char*)ctx->cmd + (pt2 - ctx->cmd_t);
    if(f_open(&fil1, f1, FA_READ) != FR_OK) {
        fgoutf(ctx->pstderr, "Unable to open file: '%s'\n", f1);
        return 2;
    }
    if (f_open(&fil2, f2, FA_CREATE_NEW | FA_WRITE) != FR_OK) {
        fgoutf(ctx->pstderr, "Unable to open file to write: '%s'\n", f2);
        f_close(&fil1);
        return 3;
    }
    UINT rb, wb;
    while(f_read(&fil1, buf, 512, &rb) == FR_OK && rb > 0) {
        if (f_write(&fil2, buf, rb, &wb) != FR_OK) {
            fgoutf(ctx->pstderr, "Unable to write to file: '%s'\n", f2);
            goutf("%d %d\n", rb, wb);
        }
    }
    f_close(&fil2);
    f_close(&fil1);
    return 0;
}

int __required_m_api_verion() {
    return 2;
}
