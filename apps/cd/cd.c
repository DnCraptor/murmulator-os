#include "m-os-api.h"

int main() {
    cmd_ctx_t* ctx = get_cmd_ctx();
    if (ctx->argc == 1) {
        fgoutf(ctx->std_err, "Unable to change directoy to nothing\n");
        return 1;
    }
    if (ctx->argc > 2) {
        fgoutf(ctx->std_err, "Unable to change directoy to more than one target\n");
        return 1;
    }
    char* d = ctx->argv[1];
    if (strcmp(d, "\\") == 0 || strcmp(d, "/") == 0) {
        goto a;
    }
    FILINFO* pfileinfo = (FILINFO*)malloc(sizeof(FILINFO));
    if (f_stat(d, pfileinfo) != FR_OK || !(pfileinfo->fattrib & AM_DIR)) {
        free(pfileinfo);
        fgoutf(ctx->std_err, "Unable to find directory: '%s'\n", d);
        return 1;
    }
    free(pfileinfo);
a:
    set_ctx_var(ctx, "CD", d);
    return 0;
}
