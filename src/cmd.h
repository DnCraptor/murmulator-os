#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include "ff.h"

size_t get_heap_total();
char* get_curr_dir();
FIL * get_stdout();
FIL * get_stderr();
typedef struct {
    char* cmd;
    char* cmd_t; // tokenised
    int tokens;
    char* curr_dir;
    FIL * pstdout;
    FIL * pstderr;
    char* path;
    int ret_code;
    char* base;
} cmd_startup_ctx_t;
cmd_startup_ctx_t* init_ctx();
cmd_startup_ctx_t* get_cmd_startup_ctx();
char* next_token(char* t);
char* concat(const char* s1, const char* s2);
char* exists(cmd_startup_ctx_t* ctx);
char* concat2(const char* s1, size_t sz, const char* s2);

#ifdef __cplusplus
}
#endif
