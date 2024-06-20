#pragma once
#include <stdbool.h>
#include "ff.h"
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
} cmd_startup_ctx_t;
cmd_startup_ctx_t* get_cmd_startup_ctx();
char* next_token(char* t);
