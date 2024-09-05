#include "cmd.h"
#include <string.h>

const char TEMP[] = "TEMP";
const char _mc_con[] = ".mc.con";
const char _cmd_history[] = ".cmd_history";

static cmd_ctx_t ctx = { 0 };
static size_t TOTAL_HEAP_SIZE = configTOTAL_HEAP_SIZE;

size_t get_heap_total() {
    return TOTAL_HEAP_SIZE;
}
char* copy_str(const char* s) {
    char* res = (char*)pvPortMalloc(strlen(s) + 1);
    strcpy(res, s);
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
    if (src->vars_num && src->vars) {
        res->vars = (vars_t*)pvPortMalloc( sizeof(vars_t) * src->vars_num );
        res->vars_num = src->vars_num;
        for (size_t i = 0; i < src->vars_num; ++i) {
            if (src->vars[i].value) {
                res->vars[i].value = copy_str(src->vars[i].value);
            }
            res->vars[i].key = src->vars[i].key; // const
        }
    }
    res->pboot_ctx = src->pboot_ctx; src->pboot_ctx = 0;
    res->prev = src->prev; src->prev = 0;
    res->next = src->next; src->next = 0;
    res->stage = src->stage;
    res->ret_code = src->ret_code;
    res->user_data = 0;
    res->forse_flash = false;
    return res;
}
void cleanup_ctx(cmd_ctx_t* src) {
    // goutf("cleanup_ctx: [%p]\n", src);
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
    src->prev = 0;
    src->next = 0;
    src->stage = INITIAL;
    if (src->user_data) {
        vPortFree(src->user_data);
        src->user_data = 0;
    }
    src->forse_flash = false;
    // gouta("cleanup_ctx <<\n");
}
void remove_ctx(cmd_ctx_t* src) {
    if (!src) return;
    // goutf("remove_ctx: [%p]\n", src);
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
    if (src->vars) {
        for (size_t i = 0; i < src->vars_num; ++i) {
            if (src->vars[i].value) {
                vPortFree(src->vars[i].value);
            }
        }
        vPortFree(src->vars);
    }
    cleanup_bootb_ctx(src);
    src->next = 0; // each pipe should remove it by self
    if (src->user_data) {
        vPortFree(src->user_data);
    }
    vPortFree(src);
    // gouta("remove_ctx <<\n");
}
cmd_ctx_t* get_cmd_startup_ctx() {
    return &ctx;
}
cmd_ctx_t* get_cmd_ctx() {
    const TaskHandle_t th = xTaskGetCurrentTaskHandle();
    return (cmd_ctx_t*) pvTaskGetThreadLocalStoragePointer(th, 0);
}
FIL* get_stdout() {
    cmd_ctx_t* pctx = get_cmd_ctx();
    return pctx ? pctx->std_out : ctx.std_out;
}
FIL* get_stderr() {
    cmd_ctx_t* pctx = get_cmd_ctx();
    return pctx ? pctx->std_err : ctx.std_err;
}
char* get_curr_dir() {
    cmd_ctx_t* pctx = get_cmd_ctx();
    return get_ctx_var(pctx ? pctx : &ctx, "CD");
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
    //if (strcmp(key, "CD") == 0) {
    //    f_chdir(val);
    //}
  //  taskENTER_CRITICAL();
    for (size_t i = 0; i < ctx->vars_num; ++i) {
        if (0 == strcmp(key, ctx->vars[i].key)) {
            if( ctx->vars[i].value ) {
                vPortFree(ctx->vars[i].value);
            }
            ctx->vars[i].value = copy_str(val);
            // goutf("%d/%d %s=%s\n", i+1, ctx->vars_num, key, ctx->vars[i].value);
   //         taskEXIT_CRITICAL();
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
    // goutf("%d/%d %s=%s\n", ctx->vars_num, ctx->vars_num, key, ctx->vars[ctx->vars_num - 1].value);
   // taskEXIT_CRITICAL();
}

char* get_ctx_var(cmd_ctx_t* ctx, const char* key) {
    taskENTER_CRITICAL();
    char* res = NULL;
    for (size_t i = 0; i < ctx->vars_num; ++i) {
        if (0 == strcmp(key, ctx->vars[i].key)) {
            res = ctx->vars[i].value;
            break;
        }
    }
    taskEXIT_CRITICAL();
    return res;
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
    if (ctx->argc == 0) {
        return false;
    }
    char* res = 0;
    char * cmd = ctx->argv[0];
    // goutf("[%p] cmd: %s\n", ctx, cmd);
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
    res = create_and_test( get_ctx_var(ctx, "CD"), cmd, pfileinfo);
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
        ctx->orig_cmd = res;
        ctx->stage = FOUND;
    }
    if (ctx->next) {
        if(!exists(ctx->next)) {
            return false;
        }
    }
    return res != 0;
}

#include "sound.h"
#include "graphics.h"

inline static void type_char(string_t* s_cmd, char c) {
    __putc(c);
    string_push_back_c(s_cmd, c);
}

inline static char* next_on(char* l, char *bi, bool in_quotas) {
    char *b = bi;
    while(*l && *b && *l == *b) {
        if (*b == ' ' && !in_quotas) break;
        l++;
        b++;
    }
    if (*l == 0 && !in_quotas) {
        char* bb = b;
        while(*bb) {
            if (*bb == ' ') {
                return bi;
            }
            bb++;
        }
    }
    return *l == 0 ? b : bi;
}

void cmd_tab(cmd_ctx_t* ctx, string_t* s_cmd) {
    char * p = s_cmd->p;
    char * p2 = p;
    bool in_quotas = false;
    while (*p) {
        char c = *p++;
        if (c == '"') {
            p2 = p;
            in_quotas = true;
            break;
        }
        if (c == ' ') {
            p2 = p;
        }
    }
    p = p2;
    char * p3 = p2;
    while (*p3) {
        if (*p3++ == '/') {
            p2 = p3;
        }
    }
    string_t* s_b = NULL;
    if (p != p2) {
        s_b = new_string_cc(p);
        string_resize(s_b, p2 - p);
    } else {
        s_b = new_string_v();
    }
    DIR* pdir = (DIR*)pvPortMalloc(sizeof(DIR));
    FILINFO* pfileInfo = (FILINFO*)pvPortMalloc(sizeof(FILINFO));
    //goutf("\nDIR: %s\n", p != p2 ? b : curr_dir);
    if (FR_OK != f_opendir(pdir, p != p2 ? s_b->p : get_ctx_var(ctx, "CD"))) {
        delete_string(s_b);
        return;
    }
    int total_files = 0;
    while (f_readdir(pdir, pfileInfo) == FR_OK && pfileInfo->fname[0] != '\0') {
        p3 = next_on(p2, pfileInfo->fname, in_quotas);
        if (p3 != pfileInfo->fname) {
            string_replace_cs(s_b, p3);
            total_files++;
            break; // TODO: variants
        }
    }
    if (total_files == 1) {
        p3 = s_b->p;
        while (*p3) {
            type_char(s_cmd, *p3++);
        }
        if (in_quotas) {
            type_char(s_cmd, '"');
        }
    } else {
        blimp(10, 5);
    }
    delete_string(s_b);
    f_closedir(pdir);
    vPortFree(pfileInfo);
    vPortFree(pdir);
}

int history_steps(cmd_ctx_t* ctx, int cmd_history_idx, string_t* s_cmd) {
    char* tmp = get_ctx_var(ctx, TEMP);
    if(!tmp) tmp = "";
    size_t cdl = strlen(tmp);
    char * cmd_history_file = concat(tmp, _cmd_history);
    FIL* pfh = (FIL*)pvPortMalloc(sizeof(FIL));
    int idx = 0;
    UINT br;
    f_open(pfh, cmd_history_file, FA_READ);
    char* b = (char*)pvPortMalloc(512);
    while(f_read(pfh, b, 512, &br) == FR_OK && br) {
        for(size_t i = 0; i < br; ++i) {
            char t = b[i];
            if(t == '\n') { // next line
                if(cmd_history_idx == idx)
                    break;
                string_resize(s_cmd, 0);
                idx++;
            } else {
                string_push_back_c(s_cmd, t);
            }
        }
    }
    vPortFree(b);
    f_close(pfh);
    vPortFree(pfh);
    vPortFree(cmd_history_file);
    return idx;
}


inline static char replace_spaces0(char t) {
    return (t == ' ') ? 0 : t;
}

inline static size_t in_quotas(size_t i, string_t* pcmd, string_t* t) {
    for (; i < pcmd->size; ++i) {
        char c = pcmd->p[i];
        if (c == '"') {
            return i;
        }
        string_push_back_c(t, c);
    }
    return i;
}

#include "../../api/m-os-api-c-list.h"

inline static void tokenize_cmd(list_t* lst, string_t* pcmd, cmd_ctx_t* ctx) {
    while (pcmd->size && pcmd->p[0] == ' ') { // remove trailing spaces
        string_clip(pcmd, 0);
    }
    if (!pcmd->size) {
        return 0;
    }
    bool in_space = false;
    int inTokenN = 0;
    string_t* t = new_string_v();
    for (size_t i = 0; i < pcmd->size; ++i) {
        char c = pcmd->p[i];
        if (c == '"') {
            if(t->size) {
                list_push_back(lst, t);
                t = new_string_v();
            }
            i = in_quotas(++i, pcmd, t);
            in_space = false;
            continue;
        }
        c = replace_spaces0(c);
        if (in_space) {
            if(c) { // token started
                in_space = false;
                list_push_back(lst, t);
                t = new_string_v();
            }
        } else if(!c) { // not in space, after the token
            in_space = true;
        }
        string_push_back_c(t, c);
    }
    if (t->size) list_push_back(lst, t);
}

inline static void cmd_write_history(cmd_ctx_t* ctx, string_t* s_cmd) {
    char* tmp = get_ctx_var(ctx, TEMP);
    if(!tmp) tmp = "";
    size_t cdl = strlen(tmp);
    char * cmd_history_file = concat(tmp, _cmd_history);
    FIL* pfh = (FIL*)pvPortMalloc(sizeof(FIL));
    f_open(pfh, cmd_history_file, FA_OPEN_ALWAYS | FA_WRITE | FA_OPEN_APPEND);
    UINT br;
    f_write(pfh, s_cmd->p, s_cmd->size, &br);
    f_write(pfh, "\n", 1, &br);
    f_close(pfh);
    vPortFree(pfh);
    vPortFree(cmd_history_file);
}

inline static string_t* get_std_out1(bool* p_append, string_t* pcmd, size_t i0, size_t sz) {
    string_t* std_out_file = new_string_v();
    for (size_t i = i0; i < sz; ++i) {
        char c = pcmd->p[i];
        if (i == i0 && c == '>') {
            *p_append = true;
            continue;
        }
        if (c == '"') {
            in_quotas(++i, pcmd, std_out_file);
            break;
        }
        if (c != ' ') {
            string_push_back_c(std_out_file, c);
        }
    }
    return std_out_file;
}

inline static string_t* get_std_out0(bool* p_append, string_t* pcmd) {
    bool in_quotas = false;
    size_t sz = pcmd->size;
    for (size_t i = 0; i < sz; ++i) {
        char c = pcmd->p[i];
        if (c == '"') {
            in_quotas = !in_quotas;
        }
        if (!in_quotas && c == '>') {
            string_t* t = get_std_out1(p_append, pcmd, i + 1, sz);
            string_resize(pcmd, i); // use only left side of the string
            return t;
        }
    }
    return NULL;
}

inline static bool prepare_ctx(string_t* pcmd, cmd_ctx_t* ctx) {
    bool in_quotas = false;
    bool append = false;
    string_t* std_out_file = get_std_out0(&append, pcmd);
    if (std_out_file) {
        ctx->std_out = (FIL*)pvPortCalloc(1, sizeof(FIL));
        if (FR_OK != f_open(ctx->std_out, std_out_file->p, FA_WRITE | (append ? FA_OPEN_APPEND : FA_CREATE_ALWAYS))) {
            printf("Unable to open file: '%s'\n", std_out_file->p);
            delete_string(std_out_file);
            return false;
        }
        delete_string(std_out_file);
    }

    list_t* lst = new_list_v(new_string_v, delete_string, NULL);
    tokenize_cmd(lst, pcmd, ctx);
    if (!lst->size) {
        delete_list(lst);
        return false;
    }

    ctx->argc = lst->size;
    ctx->argv = (char**)pvPortCalloc(sizeof(char*), lst->size);
    node_t* n = lst->first;
    for(size_t i = 0; i < lst->size && n != NULL; ++i, n = n->next) {
        ctx->argv[i] = copy_str(c_str(n->data));
    }
    delete_list(lst);
    if (ctx->orig_cmd) vPortFree(ctx->orig_cmd);
    ctx->orig_cmd = copy_str(ctx->argv[0]);
    ctx->stage = PREPARED;
    return true;
}

inline static cmd_ctx_t* new_ctx(cmd_ctx_t* src) {
    cmd_ctx_t* res = (cmd_ctx_t*)pvPortCalloc(1, sizeof(cmd_ctx_t));
    if (src->vars_num && src->vars) {
        res->vars = (vars_t*)pvPortMalloc( sizeof(vars_t) * src->vars_num );
        res->vars_num = src->vars_num;
        for (size_t i = 0; i < src->vars_num; ++i) {
            if (src->vars[i].value) {
                res->vars[i].value = copy_str(src->vars[i].value);
            }
            res->vars[i].key = src->vars[i].key; // const
        }
    }
    res->stage = src->stage;
    return res;
}

bool cmd_enter_helper(cmd_ctx_t* ctx, string_t* s_cmd) {
    if (s_cmd->size) {
        cmd_write_history(ctx, s_cmd);
    } else {
        goto r2;
    }
    bool exit = false;
    bool in_qutas = false;
    cmd_ctx_t* ctxi = ctx;
    string_t* t = new_string_v();
    for (size_t i = 0; i <= s_cmd->size; ++i) {
        char c = s_cmd->p[i];
        if (!c) {
            exit = prepare_ctx(t, ctxi);
            break;
        }
        if (c == '"') {
            in_qutas = !in_qutas;
        }
        if (!in_qutas && c == '|') {
            cmd_ctx_t* curr = ctxi;
            cmd_ctx_t* next = new_ctx(ctxi);
            exit = prepare_ctx(t, curr);
            curr->std_out = (FIL*)pvPortCalloc(1, sizeof(FIL));
            curr->std_err = curr->std_out;
            next->std_in = (FIL*)pvPortCalloc(1, sizeof(FIL));
            f_open_pipe(curr->std_out, next->std_in);
            curr->detached = true;
            next->prev = curr;
            curr->next = next;
            next->detached = false;
            ctxi = next;
            string_resize(t, 0);
            continue;
        }
        if (!in_qutas && c == '&') {
            exit = prepare_ctx(t, ctxi);
            ctxi->detached = true;
            string_resize(t, 0);
            break;
        }
        string_push_back_c(t, c);
    }
    delete_string(t);
    if (exit) { // prepared ctx
        return true;
    }
    ctxi = ctx->next;
    ctx->next = 0;
    while(ctxi) { // remove pipe chain
        cmd_ctx_t* next = ctxi->next;
        remove_ctx(ctxi);
        ctxi = next;
    }
    cleanup_ctx(ctx); // base ctx to be there
r2:
    return false;
}

void op_console(cmd_ctx_t* ctx, FRFpvUpU_ptr_t fn, BYTE mode) {
    char* tmp = get_ctx_var(ctx, TEMP);
    if(!tmp) tmp = "";
    size_t cdl = strlen(tmp);
    char * mc_con_file = concat(tmp, _mc_con);
    FIL* pfh = (FIL*)pvPortMalloc(sizeof(FIL));
    if (FR_OK != f_open(pfh, mc_con_file, mode)) {
        goto r;
    }
    size_t sz = (get_screen_width() * get_screen_height() * get_screen_bitness()) >> 3;
    UINT rb;
    fn(pfh, get_buffer(), sz, &rb);
    f_close(pfh);
r:
    vPortFree(pfh);
    vPortFree(mc_con_file);
}

bool f_read_str(FIL* f, char* buf, size_t lim) {
    UINT br;
    if (f_read(f, buf, lim, &br) != FR_OK || br == 0) {
        return false;
    }
    if (buf[0] == '\r') {
        for (size_t i = 1; i < br; ++i) {
            buf[i - 1] = buf[i];
            if(buf[i] == '\n') {
               if (i != 1 && buf[i - 2] == '\r') buf[i - 2] = 0;
                buf[i - 1] = 0;
                f_lseek(f, f_tell(f) + i - br);
                return true;
            }
        }
    }
    for (size_t i = 0; i < br; ++i) {
        if(buf[i] == '\n') {
            if (i != 0 && buf[i - 1] == '\r') buf[i - 1] = 0;
            buf[i] = 0;
            f_lseek(f, f_tell(f) + i + 1 - br);
            return true;
        }
    }
    buf[br - 1] = 0;
    f_lseek(f, f_tell(f) - 1);
    return true;
}
