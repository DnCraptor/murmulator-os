#include "m-os-api.h"

int main() {
    cmd_ctx_t* ctx = get_cmd_ctx();
    if (ctx->argc == 1) {
        fgoutf(ctx->std_err, "Unable to kill processs with not specified number\n");
        return 1;
    }
    if (ctx->argc > 2) {
        fgoutf(ctx->std_err, "Unable to kill more than one process\n");
        return 1;
    }
    uint32_t ps_num = atoi(ctx->argv[1]);
    int res = kill(ps_num);
    if (res == 0) {
        fprintf(ctx->std_err, "Task #%d not found.\n", ps_num);
    }
    else if (res == 1) {
        fprintf(ctx->std_err, "Task #%d has no SIGTERM action support (process state was set to SIGTERM).\n", ps_num);
    }
    else if (res == 2) {
        printf("Task #%d - SIGTERM was sent.\n", ps_num);
    }
    else if (res == 3) {
        printf("Task #%d - cmd context is not defined.\n", ps_num);
    }
    return 0;
}
