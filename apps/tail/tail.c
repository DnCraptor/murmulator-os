#include "m-os-api.h"

static volatile bool ctrlPressed = false;
static volatile bool cPressed = false;

void* memset(void* p, int v, size_t sz) {
    typedef void* (*fn)(void *, int, size_t);
    return ((fn)_sys_table_ptrs[142])(p, v, sz);
}

void* memcpy(void *__restrict dst, const void *__restrict src, size_t sz) {
    typedef void* (*fn)(void *, const void*, size_t);
    return ((fn)_sys_table_ptrs[167])(dst, src, sz);
}

int _init(void) {
    ctrlPressed = false;
    cPressed = false;
}

static scancode_handler_t scancode_handler;

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
    if(!len) {
        fprintf(ctx->std_err, "Unexpected argument: %s\n", ctx->argv[2]);
        return 3;
    }
    uint32_t MAX_WIDTH = ctx->detached ? 255 : get_console_width();
    uint32_t MAX_LEN = MAX_WIDTH + 1;
    char* p = calloc(MAX_LEN, len);
    FIL* f = (FIL*)malloc(sizeof(FIL));
    if (f_open(f, fn, FA_READ) != FR_OK) {
        free(f);
        free(p);
        fprintf(ctx->std_err, "Unable to open file: '%s'\n", fn);
        return 2;
    }
    char* buf = malloc(300);
    size_t curr_line = 0; size_t curr_pos = 0;
    size_t rb;
    while(f_read(f, buf, 299, &rb) == FR_OK && rb > 0) {
        for (size_t i = 0; i < rb; ++i) {
            char c = buf[i];
            if (c == '\r') continue;
            if (c == '\n') {
                curr_line++;
                curr_pos = 0;
            } else {
                *(p + curr_line * MAX_LEN + curr_pos) = c ? c : ' ';
                curr_pos++;
                if (curr_pos >= MAX_WIDTH) {
                    *(p + curr_line * MAX_LEN + curr_pos) = 0;
                    curr_pos = 0;
                    curr_line++;
                }
            }
            if (curr_line >= len) {
                curr_line = 0;
            }
            *(p + curr_line * MAX_LEN + curr_pos) = 0;
        }
    }
    free(buf);
    f_close(f);
    free(f);
    for (size_t cl = curr_line; cl < len; ++cl) {
        printf("%s\n", p + cl * MAX_LEN);
    }
    for (size_t cl = 0; cl < curr_line; ++cl) {
        printf("%s\n", p + cl * MAX_LEN);
    }
    free(p);
    return 0;
}

inline static int monitor(const char * fn) {
    scancode_handler = get_scancode_handler();
    set_scancode_handler(scancode_handler_impl);

    while (!ctrlPressed && !cPressed) {
        
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

int __required_m_api_verion(void) {
    return M_API_VERSION;
}
