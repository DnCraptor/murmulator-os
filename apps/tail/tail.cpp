#include "m-os-api.h"

extern "C" void* memset(void* p, int v, size_t sz) {
    typedef void* (*fn)(void *, int, size_t);
    return ((fn)_sys_table_ptrs[142])(p, v, sz);
}

extern "C" void _ZdlPvj(void* p) { // TODO: lookup for a case
    free(p);
}

static volatile bool ctrlPressed;
static volatile bool cPressed;
static volatile scancode_handler_t scancode_handler;

static bool scancode_handler_impl(const uint32_t ps2scancode) { // core ?
    switch ((uint8_t)ps2scancode & 0xFF) {
      case 0x1D:
        ctrlPressed = true;
        break;
      case 0x9D:
        ctrlPressed = false;
        break;
      case 0x2E:
        cPressed = true;
        break;
      case 0xAE:
        cPressed = false;
        break;
      default:
        break;
    }
    if (scancode_handler) {
        return scancode_handler(ps2scancode);
    }
    return false;
}

inline static int tail(cmd_ctx_t* ctx, const char * fn, size_t len) {
    // printf("tail(%s, %d)\n", fn, len);
    if(!len) {
        fprintf(ctx->std_err, "Unexpected argument: %s\n", ctx->argv[2]);
        return 3;
    }
    FIL* f = new FIL();
    if (f_open(f, fn, FA_READ) != FR_OK) {
        free(f);
        fprintf(ctx->std_err, "Unable to open file: '%s'\n", fn);
        return 2;
    }
    uint32_t MAX_WIDTH = ctx->detached ? 255 : get_console_width();
    uint32_t MAX_LEN = MAX_WIDTH + 1;
    char* p = (char*)calloc(MAX_LEN, len);
    char* buf = new char[512];
    size_t curr_line = 0; size_t curr_pos = 0;
    size_t lines = 0;
    size_t rb;
    while(f_read(f, buf, 512, &rb) == FR_OK && rb > 0) {
        for (size_t i = 0; i < rb; ++i) {
            char c = buf[i];
            if (c == '\r') continue;
            if (c == '\n') {
                curr_line++;
                curr_pos = 0;
                lines++;
            } else {
                *(p + curr_line * MAX_LEN + curr_pos) = c ? c : ' ';
                curr_pos++;
                if (curr_pos >= MAX_WIDTH) {
                    curr_line++;
                    curr_pos = 0;
                }
            }
            if (curr_line >= len) {
                curr_line = 0;
            }
            *(p + curr_line * MAX_LEN + curr_pos) = 0;
        }
    }
    delete[] buf;
    f_close(f);
    delete f;
    printf("Total lines: %d; show lines: %d\n", lines, len);
    for (size_t cl = curr_line; cl < len; ++cl) {
        const char* s = p + cl * MAX_LEN;
        if (strlen(s))
            printf("%s\n", s);
    }
    for (size_t cl = 0; cl < curr_line; ++cl) {
        printf("%s\n", p + cl * MAX_LEN);
    }
    free(p);
    return 0;
}

inline static int monitor(const char * fn) {
    ctrlPressed = false;
    cPressed = false;
    scancode_handler = get_scancode_handler();
    set_scancode_handler(scancode_handler_impl);

    while (!ctrlPressed || !cPressed) {
        
    }    

    set_scancode_handler(scancode_handler);
    return 0;
}

int main(void) {
    cmd_ctx_t* ctx = get_cmd_ctx();
    if (ctx->argc == 0) {
        fprintf(ctx->std_err, "ATTTENTION! BROKEN EXECUTION CONTEXT [%p]!\n", ctx);
        return -1;
    }
    if (ctx->argc == 1) {
        fgoutf(ctx->std_err, "Unable to type file tail without a file\n");
        return 1;
    }
    if (ctx->argc == 2) { // tail [file]
        return tail(ctx, ctx->argv[1], 10);
    }
    if (ctx->argc == 3) { // tail -f [file]
        if (strncmp(ctx->argv[1], "-f", 2) == 0 || strncmp(ctx->argv[1], "--follow", 8) == 0) {
            return monitor(ctx->argv[2]);
        }
        goto e2;
    }
    if (ctx->argc == 4) { // tail -n ## [file]
        if (strncmp(ctx->argv[1], "-n", 2) == 0) {
            return tail(ctx, ctx->argv[3], atoi(ctx->argv[2]));
        }
        goto e2;
    }
    fprintf(ctx->std_err, "Unexpected set of arguments\n");
    return 4;
e2:
    fprintf(ctx->std_err, "Unexpected argument: %s\n", ctx->argv[1]);
    return 2;
}

extern "C" int __required_m_api_verion(void) {
    return M_API_VERSION;
}
