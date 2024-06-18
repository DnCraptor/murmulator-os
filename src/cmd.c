// mainly moved to separate app
#include <stdint.h>
#include <stddef.h>
#include "cmd.h"

static char curr_dir[512] = "MOS"; // TODO: configure start dir
static FIL f0 = {0}; // output (stdout)
static FIL f1 = {0}; // stderr

FIL * get_stdout() {
    return &f0;
}
FIL * get_stderr() {
    return &f1;
}

static cmd_startup_ctx_t cmd_startup_ctx = { 0, 0, 0, curr_dir, &f0, &f1 }; // TODO: per process
cmd_startup_ctx_t * get_cmd_startup_ctx() {
    return &cmd_startup_ctx;
}

char* get_curr_dir() {
    return curr_dir;
}

char* next_token(char* t) {
    char *t1 = t + strlen(t);
    while(!*t1++);
    return t1 - 1;
}
