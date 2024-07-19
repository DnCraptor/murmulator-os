#include "m-os-api.h"

inline static void hex(uint32_t off, const char* buf, UINT rb) {
    for (unsigned i = 0; i < rb; i += 16) {
        printf("%08X  ", off + i);
        for (unsigned j = 0; j < 16; ++j) {
            if (j + i < rb) {
                printf("%02X ", buf[i + j]);
            } else {
                print("   ");
            }
            if(j == 7) {
                print(" ");
            }
        }
        print("  ");
        for (unsigned j = 0; j < 16 && j + i < rb; ++j) {
            printf("%c", buf[i + j] == '\n' ? ' ' : buf[i + j]);
        }
        print("\n");
    }
    print("\n");
}

int main() {
    cmd_ctx_t* ctx = get_cmd_ctx();
    if (ctx->argc == 0) {
        fprintf(ctx->std_err, "ATTTENTION! BROKEN EXECUTION CONTEXT [%p]!\n", ctx);
        return -1;
    }
    if (ctx->argc == 1) {
        fprintf(ctx->std_err, "Unable to hex file with no name\n");
        return 1;
    }
    if (ctx->argc > 2) {
        fprintf(ctx->std_err, "Unable to hex more than one file\n");
        return 1;
    }
    char* fn = ctx->argv[1];
    FIL* f = (FIL*)pvPortMalloc(sizeof(FIL));
    if (f_open(f, fn, FA_READ) != FR_OK) {
        vPortFree(f);
        fprintf(ctx->std_err, "Unable to open file: '%s'\n", fn);
        return 2;
    }
    UINT rb;
    char* buf = (char*)pvPortMalloc(512);
    uint32_t off = 0;
    while(f_read(f, buf, 512, &rb) == FR_OK && rb > 0) {
        hex(off, buf, rb);
        off += rb;
    }
    vPortFree(buf);
    f_close(f);
    vPortFree(f);
    return 0;
}

int __required_m_api_verion(void) {
    return M_API_VERSION;
}
