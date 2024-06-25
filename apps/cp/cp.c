#include "m-os-api.h"

int main(void) {
    cmd_ctx_t* ctx = get_cmd_ctx();
    if (ctx->argc < 3) {
        fgoutf(ctx->std_err, "Usage: %s <file1> <file2>\n", ctx->argv[0]);
        return 1;
    }
    FIL* fil1 = malloc(sizeof(FIL));
    if(f_open(fil1, ctx->argv[1], FA_READ) != FR_OK) {
        fgoutf(ctx->std_err, "Unable to open file: '%s'\n", ctx->argv[1]);
        free(fil1);
        return 2;
    }
    FIL* fil2 = malloc(sizeof(FIL));
    if (f_open(fil2, ctx->argv[2], FA_CREATE_NEW | FA_WRITE) != FR_OK) {
        fgoutf(ctx->std_err, "Unable to open file to write: '%s'\n", ctx->argv[2]);
        f_close(fil1);
        free(fil2);
        free(fil1);
        return 3;
    }
    char* buf = malloc(512);
    UINT rb, wb;
    while(f_read(fil1, buf, 512, &rb) == FR_OK && rb > 0) {
        if (f_write(fil2, buf, rb, &wb) != FR_OK) {
            fgoutf(ctx->std_err, "Unable to write to file: '%s'\n", ctx->argv[2]);
            goutf("%d %d\n", rb, wb);
        }
    }
    free(buf);
    free(fil2);
    free(fil1);
    f_close(fil2);
    f_close(fil1);
    return 0;
}

int __required_m_api_verion(void) {
    return M_API_VERSION;
}
