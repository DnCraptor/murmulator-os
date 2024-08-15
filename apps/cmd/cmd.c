#include "m-os-api.h"
#include "m-os-api-sdtfn.h"

static string_t* s_cmd = NULL;

inline static void cmd_backspace() {
    if (s_cmd->size == 0) {
        blimp(10, 5);
        return;
    }
    string_resize(s_cmd, s_cmd->size - 1);
    gbackspace();
}

inline static void type_char(char c) {
    putc(c);
    string_push_back_c(s_cmd, c);
}

inline static char replace_spaces0(char t) {
    return (t == ' ') ? 0 : t;
}

inline static int tokenize_cmd(char* cmdt, cmd_ctx_t* ctx) {
    while (!cmdt[0 && cmdt[0] == ' ']) ++cmdt; // ignore trailing spaces
    if (cmdt[0] == 0) {
        return 0;
    }
    if (ctx->orig_cmd) free(ctx->orig_cmd);
    ctx->orig_cmd = copy_str(cmdt);
    //goutf("orig_cmd: '%s' [%p]; cmd: '%s' [%p]\n", ctx->orig_cmd, ctx->orig_cmd, cmdt, cmdt);
    bool in_space = true;
    bool in_qutas = false;
    int inTokenN = 0;
    char* t1 = ctx->orig_cmd;
    char* t2 = cmdt;
    while(*t1) {
        if (*t1 == '"') in_qutas = !in_qutas;
        if (in_qutas) {
            *t2++ = *t1++;
            continue; 
        }
        char c = replace_spaces0(*t1++);
        //goutf("%02X -> %c %02X; t1: '%s' [%p], t2: '%s' [%p]\n", c, *t2, *t2, t1, t1, t2, t2);
        if (in_space) {
            if(c) { // token started
                in_space = 0;
                inTokenN++; // new token
            }
        } else if(!c) { // not in space, after the token
            in_space = true;
        }
        *t2++ = c;
    }
    *t2 = 0;
    //goutf("cmd: %s\n", cmd);
    return inTokenN;
}

inline static void cmd_write_history(cmd_ctx_t* ctx) {
    char* tmp = get_ctx_var(ctx, "TEMP");
    if(!tmp) tmp = "";
    size_t cdl = strlen(tmp);
    char * cmd_history_file = concat(tmp, ".cmd_history");
    FIL* pfh = (FIL*)malloc(sizeof(FIL));
    f_open(pfh, cmd_history_file, FA_OPEN_ALWAYS | FA_WRITE | FA_OPEN_APPEND);
    UINT br;
    f_write(pfh, s_cmd->p, s_cmd->size, &br);
    f_write(pfh, "\n", 1, &br);
    f_close(pfh);
    free(pfh);
    free(cmd_history_file);
}

inline static bool prepare_ctx(char* cmdt, cmd_ctx_t* ctx) {
    char* t = cmdt;
    bool in_quotas = false;
    bool append = false;
    char* std_out = 0;
    while (*t) {
        if (*t == '"') in_quotas = !in_quotas;
        if (!in_quotas && *t == '>') {
            *t++ = 0;
            if (*t == '>') {
                *t++ = 0;
                append = true;
                std_out = t;
            } else {
                std_out = t;
            }
            break;
        }
        t++;
    }
    if (std_out) {
        char* b = std_out;
        in_quotas = false;
        bool any_legal = false;
        while(*b) {
            if (!in_quotas && *b == ' ') {
                if (any_legal) {
                    *b = 0;
                    break;
                }
                std_out = b + 1;
            } else if (*b == '"') {
                if (in_quotas) {
                    *b = 0;
                    break;
                } else {
                    std_out = b + 1;
                }
                in_quotas = !in_quotas;
            } else {
                any_legal = true;
            }
            b++;
        }
        ctx->std_out = calloc(1, sizeof(FIL));
        if (FR_OK != f_open(ctx->std_out, std_out, FA_WRITE | (append ? FA_OPEN_APPEND : FA_CREATE_ALWAYS))) {
            printf("Unable to open file: '%s'\n", std_out);
            return false;
        }
    }

    int tokens = tokenize_cmd(cmdt, ctx);
    if (tokens == 0) {
        return false;
    }

    ctx->argc = tokens;
    ctx->argv = (char**)malloc(sizeof(char*) * tokens);
    t = cmdt;
    while (!t[0 && t[0] == ' ']) ++t; // ignore trailing spaces
    for (uint32_t i = 0; i < tokens; ++i) {
        ctx->argv[i] = copy_str(t);
        t = next_token(t);
        char *q = t;
        bool in_quotas = false;
        while (*q) {
            if (*q == '"') {
                if(in_quotas) {
                    *q = 0;
                    break;
                }
                else t = q + 1;
                in_quotas = !in_quotas;
            }
            q++;
        }
        
    }
    ctx->stage = PREPARED;
    return true;
}

inline static cmd_ctx_t* new_ctx(cmd_ctx_t* src) {
    cmd_ctx_t* res = (cmd_ctx_t*)pvPortMalloc(sizeof(cmd_ctx_t));
    memset(res, 0, sizeof(cmd_ctx_t));
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

inline static void prompt(cmd_ctx_t* ctx) {
    graphics_set_con_color(13, 0);
    printf("[%s]", get_ctx_var(ctx, "CD"));
    graphics_set_con_color(7, 0);
    printf("$ ");
}

inline static bool cmd_enter(cmd_ctx_t* ctx) {
    putc('\n');
    if (s_cmd->size) {
        cmd_write_history(ctx);
    } else {
        goto r2;
    }

    char* tc = s_cmd->p;
    char* ts = tc;
    bool exit = false;
    bool in_qutas = false;
    cmd_ctx_t* ctxi = ctx;
    while(1) {
        if (!*tc) {
            //printf("'%s' by end zero\n", ts);
            exit = prepare_ctx(ts, ctxi);
            break;
        } else if (*tc == '"') {
            in_qutas = !in_qutas;
        } else if (!in_qutas && *tc == '|') {
            //printf("'%s' by pipe\n", ts);
            *tc = 0;
            cmd_ctx_t* curr = ctxi;
            cmd_ctx_t* next = new_ctx(ctxi);
            exit = prepare_ctx(ts, curr);
            curr->std_out = calloc(1, sizeof(FIL));
            curr->std_err = curr->std_out;
            next->std_in = calloc(1, sizeof(FIL));
            f_open_pipe(curr->std_out, next->std_in);
            curr->detached = true;
            next->prev = curr;
            curr->next = next;
            next->detached = false;
            ctxi = next;
            ts = tc + 1;
        } else if (!in_qutas && *tc == '&') {
            //printf("'%s' detached\n", ts);
            *tc = 0;
            exit = prepare_ctx(ts, ctxi);
            ctxi->detached = true;
            break;
        }
        tc++;
    }
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
    prompt(ctx);
    string_resize(s_cmd, 0);
    return false;
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

inline static void cmd_tab(cmd_ctx_t* ctx) {
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
        new_string_v();
    }
    DIR* pdir = (DIR*)malloc(sizeof(DIR));
    FILINFO* pfileInfo = (FILINFO*)malloc(sizeof(FILINFO));
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
        //goutf("p3: %s; p2: %s; fn: %s; cmd_t: %s; fls: %d\n", p3, p2, fileInfo.fname, b, total_files);
    }
    if (total_files == 1) {
        p3 = s_b->p;
        while (*p3) {
            type_char(*p3++);
        }
        if (in_quotas) {
            type_char('"');
        }
    } else {
        blimp(10, 5);
    }
    delete_string(s_b);
    f_closedir(pdir);
    free(pfileInfo);
    free(pdir);
}

inline static void cancel_entered() {
    while(s_cmd->size) {
        string_resize(s_cmd, s_cmd->size - 1);
        gbackspace();
    }
}

static int cmd_history_idx;

inline static int history_steps(cmd_ctx_t* ctx) {
    char* tmp = get_ctx_var(ctx, "TEMP");
    if(!tmp) tmp = "";
    size_t cdl = strlen(tmp);
    char * cmd_history_file = concat(tmp, ".cmd_history");
    FIL* pfh = (FIL*)malloc(sizeof(FIL));
    int idx = 0;
    UINT br;
    f_open(pfh, cmd_history_file, FA_READ);
    char* b = malloc(512);
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
    free(b);
    f_close(pfh);
    free(pfh);
    free(cmd_history_file);
    return idx;
}

inline static void cmd_up(cmd_ctx_t* ctx) {
    cancel_entered();
    cmd_history_idx--;
    int idx = history_steps(ctx);
    if (cmd_history_idx < 0) cmd_history_idx = idx;
    gouta(s_cmd->p);
}

inline static void cmd_down(cmd_ctx_t* ctx) {
    cancel_entered();
    if (cmd_history_idx == -2) cmd_history_idx = -1;
    cmd_history_idx++;
    history_steps(ctx);
    gouta(s_cmd->p);
}

int main(void) {
    show_logo(false);
    cmd_ctx_t* ctx = get_cmd_ctx();
    cleanup_ctx(ctx);
    s_cmd = new_string_v();
    prompt(ctx);
    cmd_history_idx = -2;
    while(1) {
        char c = getch();
        if (c) {
            if (c == 8) cmd_backspace();
            else if (c == 17) cmd_up(ctx);
            else if (c == 18) cmd_down(ctx);
            else if (c == '\t') cmd_tab(ctx);
            else if (c == '\n') {
                if ( cmd_enter(ctx) ) {
                    delete_string(s_cmd);
                    //goutf("[%s]EXIT to exec, stage: %d\n", ctx->curr_dir, ctx->stage);
                    return 0;
                }
            } else type_char(c);
        }
    }
    __unreachable();
}

int __required_m_api_verion(void) {
    return M_API_VERSION;
}
