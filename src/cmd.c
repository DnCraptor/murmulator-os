#include "cmd.h"
#include <string.h>
#include "FreeRTOS.h"

static cmd_startup_ctx_t ctx = { 0 };

cmd_startup_ctx_t* init_ctx() {
    ctx.cmd = (char*)pvPortMalloc(512); ctx.cmd[0] = 0;
    ctx.cmd_t = (char*)pvPortMalloc(512); ctx.cmd_t[0] = 0;
    ctx.curr_dir = (char*)pvPortMalloc(512);
    ctx.pstdout = (FIL*)pvPortMalloc(sizeof(FIL)); memset(ctx.pstdout, 0, 512);
    ctx.pstderr = (FIL*)pvPortMalloc(sizeof(FIL)); memset(ctx.pstderr, 0, 512);
    ctx.path = (char*)pvPortMalloc(512);
    strcpy(ctx.curr_dir, "MOS");
    strcpy(ctx.path, "MOS");
    return &ctx;
}
cmd_startup_ctx_t * get_cmd_startup_ctx() {
    return &ctx;
}
FIL * get_stdout() {
    return ctx.pstdout;
}
FIL * get_stderr() {
    return ctx.pstderr;
}
char* get_curr_dir() {
    return ctx.curr_dir;
}
char* next_token(char* t) {
    char *t1 = t + strlen(t);
    while(!*t1++);
    return t1 - 1;
}
char* concat(const char* s1, const char* s2) {
    size_t s = strlen(s1);
    char* res = (char*)pvPortMalloc(s + strlen(s2) + 2);
    strcpy(res, s1);
    res[s] = '/';
    strcpy(res + s + 1, s2);
    return res;
}
char* concat2(const char* s1, size_t s, const char* s2) {
    char* res = (char*)pvPortMalloc(s + strlen(s2) + 2);
    strncpy(res, s1, s);
    res[s] = '/';
    strcpy(res + s + 1, s2);
    return res;
}
inline static char* copy_str(const char* s) {
    char* res = (char*)pvPortMalloc(strlen(s) + 1);
    stpcpy(res, s);
    return res;
} 
char* exists(cmd_startup_ctx_t* ctx) {
    char* res = 0;
    char * cmd = ctx->cmd_t;
    FILINFO* pfileinfo = (FILINFO*)pvPortMalloc(sizeof(FILINFO));
    bool r = f_stat(cmd, pfileinfo) == FR_OK && !(pfileinfo->fattrib & AM_DIR);
    if (r) {
        res = copy_str(cmd);
        goto r1;
    }
    char* dir = ctx->curr_dir;
    res = concat(dir, cmd);
    r = f_stat(res, pfileinfo) == FR_OK && !(pfileinfo->fattrib & AM_DIR);
    if (r) goto r1;
    vPortFree(res);
    res = 0; 
    dir = ctx->path;
    size_t sz = strlen(dir);
    char* e = dir;
    while(e++ <= dir + sz) {
        if (*e == ';' || *e == ':' || *e == ',' || *e == 0) {
            res = concat2(dir, e - dir, cmd);
            //goutf("path %s\n", res);
            r = f_stat(res, pfileinfo) == FR_OK && !(pfileinfo->fattrib & AM_DIR);
            if (r) goto r1;
            vPortFree(res);
            res = 0;
            if(!*e) goto r1;
            dir = e;
            sz = strlen(dir);
        }
    }
r1:
    vPortFree(pfileinfo);
    return res;
}
