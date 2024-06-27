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
a:
    if(ctx->curr_dir) free(ctx->curr_dir);
    ctx->curr_dir = copy_str(d);
    free(pfileinfo);
    return 0;
}

int __required_m_api_verion(void) {
    return M_API_VERSION;
}
