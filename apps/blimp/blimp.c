#include "m-os-api.h"

int main(void) {
    cmd_ctx_t* ctx = get_cmd_ctx();
    if (ctx->argc != 3) {
        fprintf(ctx->std_err, "Usage: blimp [n1] [n2]\n"
        " where # - n1 and n2 integer numbers\n"
        "  n1 - number of cycles\n"
        "  n2 - number of OS ticks\n");
        return 1;
    }
    int n1 = atoi(ctx->argv[1]);
    int n2 = atoi(ctx->argv[2]);
    blimp(n1, n2);
    return 0;
}
