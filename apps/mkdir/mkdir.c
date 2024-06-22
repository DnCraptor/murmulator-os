#include "m-os-api.h"

int main(void) {
    cmd_startup_ctx_t* ctx = get_cmd_startup_ctx();
    if (ctx->tokens == 1) {
        fgoutf(ctx->pstderr, "Unable to make directoy with no name\n");
        return 1;
    } else {
        char * d = (char*)ctx->cmd + (next_token(ctx->cmd_t) - ctx->cmd_t);
        if (f_mkdir(d) != FR_OK) {
            fgoutf(ctx->pstderr, "Unable to mkdir: '%s'\n", d);
            return 2;
        }
    }
    return 0;
}

int __required_m_api_verion(void) {
    return 2;
}
