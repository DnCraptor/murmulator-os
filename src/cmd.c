#include "cmd.h"
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"

static cmd_ctx_t ctx = { 0 };
static size_t TOTAL_HEAP_SIZE = configTOTAL_HEAP_SIZE;

size_t get_heap_total() {
    return TOTAL_HEAP_SIZE;
}
char* copy_str(const char* s) {
    char* res = (char*)pvPortMalloc(strlen(s) + 1);
    stpcpy(res, s);
    return res;
}
static FIL* copy_FIL(FIL* f) {
    FIL* res = (FIL*)pvPortMalloc(sizeof(FIL));
    memcpy(res, f, sizeof(FIL));
    return res;
}
cmd_ctx_t* clone_ctx(cmd_ctx_t* src) {
    cmd_ctx_t* res = (cmd_ctx_t*)pvPortMalloc(sizeof(cmd_ctx_t));
    memcpy(res, src, sizeof(cmd_ctx_t));
    if (src->argc && src->argv) {
        res->argc = src->argc;
        res->argv = (char**)pvPortMalloc(sizeof(char*) * res->argc);
        for(int i = 0; i < src->argc; ++i) {
            res->argv[i] = copy_str(src->argv[i]);
        }
    }
    if (src->orig_cmd) {
        res->orig_cmd = copy_str(src->orig_cmd);
    }
    // change owner
    if (src->std_in) {
        res->std_in = src->std_in;
        src->std_in = 0;
    }
    if (src->std_out) {
        res->std_out = src->std_out;
        src->std_out = 0;
    }
    if (src->std_err) {
        res->std_err = src->std_err;
        src->std_err = 0;
    }
    if (src->curr_dir) {
        res->curr_dir = copy_str(src->curr_dir);
    }
    if (src->vars_num && src->vars) {
        res->vars = (vars_t*)pvPortMalloc( sizeof(vars_t) * src->vars_num );
        res->vars_num = src->vars_num;
        for (size_t i = 0; i < src->vars_num; ++i) {
            if (src->vars[i].value) {
                res->vars[i].value = copy_str(src->vars[i].value);
            }
            res->vars[i].key = src->vars[i].key; // const
        }
        vPortFree(src->vars);
    }
    res->pipe = 0; // do not copy pipe
    res->stage = src->stage;
    res->ret_code = src->ret_code;
    return res;
}
void cleanup_ctx(cmd_ctx_t* src) {
    // goutf("src->argc: %d\n", src->argc);
    if (src->argv) {
        for(int i = 0; i < src->argc; ++i) {
            vPortFree(src->argv[i]);
        }
        vPortFree(src->argv);
    }
    src->argv = 0;
    src->argc = 0;
    if (src->orig_cmd) {
        vPortFree(src->orig_cmd);
        src->orig_cmd = 0;
    }
    if (src->std_in) {
        f_close(src->std_in);
        vPortFree(src->std_in);
        src->std_in = 0;
    }
    if (src->std_out) {
        if (src->std_err == src->std_out) {
            src->std_err = 0;
        }
        f_close(src->std_out);
        vPortFree(src->std_out);
        src->std_out = 0;
    }
    if (src->std_err) {
        f_close(src->std_err);
        vPortFree(src->std_err);
        src->std_err = 0;
    }
    src->detached = false;
    src->ret_code = 0;
    src->pipe = 0;
    src->stage = INITIAL;
    // gouta("cleanup_ctx <<\n");
}
void remove_ctx(cmd_ctx_t* src) {
    if (src->argv) {
        for(int i = 0; i < src->argc; ++i) {
            vPortFree(src->argv[i]);
        }
        vPortFree(src->argv);
    }
    if (src->orig_cmd) {
        vPortFree(src->orig_cmd);
    }
    if (src->std_in) { f_close(src->std_in); vPortFree(src->std_in); }
    if (src->std_out && src->std_out != src->std_err) { f_close(src->std_out); vPortFree(src->std_out); }
    if (src->std_err) { f_close(src->std_err); vPortFree(src->std_err); }
    if (src->curr_dir) {
        vPortFree(src->curr_dir);
    }
    if (src->vars) {
        for (size_t i = 0; i < src->vars_num; ++i) {
            if (src->vars[i].value) {
                vPortFree(src->vars[i].value);
            }
        }
        vPortFree(src->vars);
    }
    if (src->pboot_ctx) {
        cleanup_bootb_ctx(src->pboot_ctx);
        vPortFree(src->pboot_ctx);
    }
    // src->pipe = 0; // each pipe should remove it by self
    vPortFree(src);
}
cmd_ctx_t* get_cmd_startup_ctx() {
    return &ctx;
}
cmd_ctx_t* get_cmd_ctx() {
    const TaskHandle_t th = xTaskGetCurrentTaskHandle();
    return (cmd_ctx_t*) pvTaskGetThreadLocalStoragePointer(th, 0);
}
FIL* get_stdout() {
    return ctx.std_out;
}
FIL* get_stderr() {
    return ctx.std_err;
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

void set_ctx_var(cmd_ctx_t* ctx, const char* key, const char* val) {
    size_t sz = strlen(val) + 1;
    for (size_t i = 0; i < ctx->vars_num; ++i) {
        if (0 == strcmp(key, ctx->vars[i].key)) {
            if( ctx->vars[i].value ) {
                vPortFree(ctx->vars[i].value);
            }
            ctx->vars[i].value = copy_str(val);
            return;
        }
    }
    // not found
    if (ctx->vars == NULL) {
        // initial state
        ctx->vars = (vars_t*)pvPortMalloc(sizeof(vars_t));
    } else {
        vars_t* old = ctx->vars;
        ctx->vars = (vars_t*)pvPortMalloc( sizeof(vars_t) * (ctx->vars_num + 1) );
        memcpy(ctx->vars, old, sizeof(vars_t) * ctx->vars_num);
        vPortFree(old);
    }
    ctx->vars[ctx->vars_num].key = key; // const to be not reallocated
    ctx->vars[ctx->vars_num].value = copy_str(val);
    ctx->vars_num++;
}

char* get_ctx_var(cmd_ctx_t* ctx, const char* key) {
    for (size_t i = 0; i < ctx->vars_num; ++i) {
        if (0 == strcmp(key, ctx->vars[i].key)) {
            return ctx->vars[i].value;
        }
    }
    return NULL;
}

static char* create_and_test(char* dir, char* cmd, FILINFO* pfileinfo) {
    char* res;
    if (dir) {
        res = concat(dir, cmd);
        if (f_stat(res, pfileinfo) == FR_OK && !(pfileinfo->fattrib & AM_DIR)) goto r1;
        vPortFree(res);
        res = 0;
    }
r1:
    return res;
}

bool exists(cmd_ctx_t* ctx) {
    //goutf("ctx->argc: %d\n", ctx->argc);
    if (ctx->argc == 0) {
        return 0;
    }
    char* res = 0;
    char * cmd = ctx->argv[0];
    //goutf("cmd: %s\n", cmd);
    FILINFO* pfileinfo = (FILINFO*)pvPortMalloc(sizeof(FILINFO));
    bool r = f_stat(cmd, pfileinfo) == FR_OK && !(pfileinfo->fattrib & AM_DIR);
    if (r) {
        res = copy_str(cmd);
        //goutf("res %s\n", res);
        goto r1;
    }
    res = create_and_test( get_ctx_var(ctx, "BASE"), cmd, pfileinfo);
    if (res) {
        //goutf("B: %s\n", res);
        goto r1;
    }
    res = create_and_test( ctx->curr_dir, cmd, pfileinfo);
    if (res) {
        //goutf("C: %s\n", res);
        goto r1;
    }
    const char* path = get_ctx_var(ctx, "PATH");
    if (path) {
        size_t sz = strlen(path);
        char* e = path;
        while(e++ <= path + sz) {
            if (*e == ';' || *e == ':' || *e == ',' || *e == 0) {
                res = concat2(path, e - path, cmd);
                //goutf("path %s\n", res);
                r = f_stat(res, pfileinfo) == FR_OK && !(pfileinfo->fattrib & AM_DIR);
                if (r) goto r1;
                vPortFree(res);
                res = 0;
                if(!*e) goto r1;
                path = e;
                sz = strlen(path);
            }
        }
    }
r1:
    vPortFree(pfileinfo);
    if (res) {
        // goutf("Found: %s\n", res);
        if(ctx->orig_cmd) vPortFree(ctx->orig_cmd);
        ctx->orig_cmd = copy_str(res);
        ctx->stage = FOUND;
    }
    cmd_ctx_t* rc = ctx->pipe;
    while (rc) {
        if(!exists(rc)) {
            return false;
        }
        rc = ctx->pipe;    
    }
    return res != 0;
}
