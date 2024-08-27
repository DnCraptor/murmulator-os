#include "m-os-api.h"

volatile bool marked_to_exit;

int main() {
    cmd_ctx_t* ctx = get_cmd_ctx();
    if (ctx->argc == 0) {
        fgoutf(ctx->std_err, "ATTTENTION! BROKEN EXECUTION CONTEXT [%p]!\n", ctx);
        return -1;
    }
    if (ctx->argc == 1) {
        fgoutf(ctx->std_err, "Unable to type file with no name\n");
        return 1;
    }
    if (ctx->argc > 2) {
        fgoutf(ctx->std_err, "Unable to type more than one file\n");
        return 1;
    }
    char* fn = ctx->argv[1];
    FIL* f = (FIL*)pvPortMalloc(sizeof(FIL));
    if (f_open(f, fn, FA_READ) != FR_OK) {
        vPortFree(f);
        fgoutf(ctx->std_err, "Unable to open file: '%s'\n", fn);
        return 2;
    }
    UINT rb;
    char* buf = (char*)pvPortMalloc(300);
    marked_to_exit = false;
    while(f_read(f, buf, 299, &rb) == FR_OK && rb > 0 && !marked_to_exit) {
        buf[rb] = 0;
        fgoutf(ctx->std_out, buf);
    }
    vPortFree(buf);
    f_close(f);
    vPortFree(f);
    return 0;
}

int __required_m_api_verion(void) {
    return M_API_VERSION;
}

// only SIGKILL is supported for now
int signal(void) {
	marked_to_exit = true;
}
