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
        fprintf(ctx->std_err, "Task #%d has no SIGKILL support.\n", ps_num);
    }
    else if (res == 2) {
        printf("Task #%d - SIGKILL was sent.\n", ps_num);
    }
    return 0;
}

int __required_m_api_verion(void) {
    return M_API_VERSION;
}
