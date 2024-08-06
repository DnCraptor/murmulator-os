#include "m-os-api.h"

inline static void hex(uint32_t off, const char* buf, UINT rb) {
    for (unsigned i = 0; i < rb; i += 16) {
        printf("%08X  ", off + i);
        for (unsigned j = 0; j < 16; ++j) {
            if (j + i < rb) {
                printf("%02X ", buf[i + j]);
            } else {
                printf("   ");
            }
            if(j == 7) {
                printf(" ");
            }
        }
        printf("  ");
        for (unsigned j = 0; j < 16 && j + i < rb; ++j) {
            char c = buf[i + j];
            printf("%c", !c || c == '\n' ? ' ' : c);
        }
        printf("\n");
    }
    printf("\n");
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
    uint32_t off = 0;
    if (ctx->argv[1][0] == '@') {
        size_t sz = strlen(ctx->argv[1]);
        if (sz <= 1) {
            fprintf(ctx->std_err, "Unsupported argument: %s\n", ctx->argv[1]);
            return 1;
        }
        size_t n = 0;
        for (int idx = sz - 1; idx > 0; --idx) {
            char c = ctx->argv[1][idx];
            off += (c - ((c >= 'a' ? 'a' - 0xA : (c >= 'A' ? 'A' - 0xA : '0'))) & 0xF) << n;
            n += 4;
        }
        printf("Hex for addresses: %ph - %ph\n", off, off + 512);
        hex(off, (char*)0, 512); // TODO:
        return 0;
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
