#include "m-os-api.h"

int main(void) {
    cmd_ctx_t* ctx = get_cmd_ctx();
    if (ctx->argc == 1) {
        fgoutf(ctx->std_err, "Unable to make directoy with no name\n");
        return 1;
    } else {
        char * d = ctx->argv[1];
        if (f_mkdir(d) != FR_OK) {
            fgoutf(ctx->std_err, "Unable to mkdir: '%s'\n", d);
            return 2;
        }
    }
    return 0;
}
