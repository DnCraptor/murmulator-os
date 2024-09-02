#define UF2_MODE
#include "m-os-api.h"

void* ay_initsong(const char* FilePath, unsigned long sr);

int boota(void* pv) {
    cmd_ctx_t* ctx = get_cmd_ctx();
    if (ctx->argc < 2 || ctx->argc > 3) {
e0:
        fprintf(ctx->std_err,
            "Usage: ay [file] [bytes]\n"
            "  where [bytes] - optional param to reserve some RAM for other applications (512 by default).\n"
        );
        return 1;
    }
    int reserve = 512;
    if (ctx->argc == 3) {
        reserve = atoi(ctx->argv[2]);
        if (!reserve) {
            goto e0;
        }
    }
    int res = 0;
    void* p = ay_initsong(ctx->argv[1], 44100);
    if (!p) {
        fprintf(ctx->std_err, "ay_initsong returns NULL\n");
        return -1;
    }
    return res;
}

unsigned long __aligned(4096) __in_boota() boota_tbl[] = { // mark pages used by program
    boota, // 2-nd page 10002000-10003000
    boota, // 3-nd page 10003000-10004000
    boota, // 4-nd page 10004000-10005000
    boota, // 5-nd page 10005000-10006000
    0
};

// just for std linker
void main() {
    int r = boota(NULL);
    r = boota_tbl;
    (void*)r;
}
