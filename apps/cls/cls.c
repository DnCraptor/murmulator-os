#include "m-os-api.h"

int main(void) {
    cmd_ctx_t* ctx = get_cmd_ctx();
    int color = 0;
    if (ctx->argc == 2) {
        color = atoi(ctx->argv[1]);
    }
    clrScr(color & 0xFF);
    return 0;
}

int __required_m_api_verion(void) {
    return M_API_VERSION;
}
