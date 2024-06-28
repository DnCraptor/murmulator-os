#include "m-os-api.h"

int main() {
    cmd_ctx_t* ctx = get_cmd_ctx();
    if (ctx->argc == 1) {
        for (size_t i = 0; i < ctx->vars_num; ++i) {
            char* v = ctx->vars[i].value;
            fprintf(ctx->std_out, "%s=%s\n", ctx->vars[i].key, !v ? "NULL" : v);
        }
        return 0;
    }
    if (ctx->argc > 2) {
        fprintf(ctx->std_err, "Unable to set more than one variable in one command\n");
        return 1;
    }
    char* eq = ctx->argv[1];
    while(*eq) {
        if(*eq == '=') {
            break;
        }
        *eq++;
    }
    if(*eq == 0) {
        char* v = get_ctx_var(ctx, ctx->argv[1]);
        fprintf(ctx->std_out, "%s=%s\n", ctx->argv[1], !v ? "NULL" : v);
        return 0;
    }
    size_t len = eq - ctx->argv[1];
    char* k = malloc(len + 1);
    strncpy(k, ctx->argv[1], len);
    k[len] = 0;
    bool remove_key = 0 != get_ctx_var(ctx, k);
    set_ctx_var(ctx, k, eq + 1);
    if (remove_key) { // do not remove key, for case it is new record in vars
        free(k);
    }
    return 0;
}

int __required_m_api_verion(void) {
    return M_API_VERSION;
}
