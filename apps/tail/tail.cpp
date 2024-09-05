#include "m-os-api.h"

extern "C" void* memset(void* p, int v, size_t sz) {
    typedef void* (*fn)(void *, int, size_t);
    return ((fn)_sys_table_ptrs[142])(p, v, sz);
}

inline static int tail(cmd_ctx_t* ctx, const char * fn, size_t len) {
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
    string* p = new string[len + 1];
    char* buf = new char[512];
    size_t curr_line = 0;
    size_t lines = 0;
    UINT rb;
    while(f_read(f, buf, 512, &rb) == FR_OK && rb > 0 && !marked_to_exit) {
        for (size_t i = 0; i < rb; ++i) {
            char c = buf[i];
            if (c == '\r') continue;
            if (c == '\n') {
                curr_line++;
                lines++;
                if (curr_line >= len + 1) {
                    curr_line = 0;
                }
                p[curr_line] = "";
            } else {
                p[curr_line] += c ? c : ' ';
            }
        }
    }
    delete[] buf;
    f_close(f);
    delete f;
    if (marked_to_exit) {
        printf("Interrupted\n");
    }
    printf("Total lines: %d; show lines: %d\n", lines, len);
    bool empty_last = (0 == p[curr_line].size());
    for (size_t cl = (empty_last ? curr_line + 1 : curr_line); cl < (empty_last ? len + 1 : len); ++cl) {
        printf("%s\n", p[cl].c_str());
    }
    for (size_t cl = 0; cl < curr_line; ++cl) {
        printf("%s\n", p[cl].c_str());
    }
    delete[] p;
    return 0;
}

inline static int monitor(const char * fn) {
    printf("Not yet implemented mode. Press: Ctrl+C\n");
    while (!marked_to_exit) {
        vTaskDelay(100);
    }    
    return 0;
}

int main(void) {
    marked_to_exit = false;
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
