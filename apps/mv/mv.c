#include "m-os-api.h"

int main(void) {
    cmd_ctx_t* ctx = get_cmd_ctx();
    if (ctx->argc != 3) {
        fprintf(ctx->std_err, "Usage: %s [from_file_name] [to_file_name]\n", ctx->argv[0]);
        return 1;
    }
    FRESULT result = f_rename(ctx->argv[1], ctx->argv[2]);
    if (result != FR_OK) {
        fprintf(ctx->std_err, "Unable to move it: FRESULT = %d\n", result);
        return 2;
    }
    return 0;
}
