#include "m-os-api.h"
#include "m-os-api-sdtfn.h"

// #define DEBUG

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

inline static void tokenize_cmd(list_t* lst, string_t* pcmd, cmd_ctx_t* ctx) {
    #ifdef DEBUG
        printf("[tokenize_cmd] %s\n", pcmd->p);
    #endif
    while (pcmd->size && pcmd->p[0] == ' ') { // remove trailing spaces
        string_clip(pcmd, 0);
        #ifdef DEBUG
            printf("[tokenize_cmd] clip: %s\n", pcmd->p);
        #endif
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
    #ifdef DEBUG
        node_t* n = lst->first;
        while(n) {
            printf("[tokenize_cmd] n: %s\n", c_str(n->data));
            n = n->next;
        }
    #endif
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

inline static string_t* get_std_out1(bool* p_append, string_t* pcmd, size_t i0, size_t sz) {
    #ifdef DEBUG
        printf("[get_std_out1] [%s] i0: %d\n", pcmd->p + i0, i0);
    #endif
    string_t* std_out_file = new_string_v();
    for (size_t i = i0; i < sz; ++i) {
        char c = pcmd->p[i];
        #ifdef DEBUG
            printf("[get_std_out1] [%s] c: %c\n", pcmd->p + i, c);
        #endif
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
    #ifdef DEBUG
        printf("[get_std_out0] %s\n", pcmd->p);
    #endif
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
    #ifdef DEBUG
        printf("[prepare_ctx] %s\n", pcmd->p);
    #endif
    bool in_quotas = false;
    bool append = false;
    string_t* std_out_file = get_std_out0(&append, pcmd);
    if (std_out_file) {
        #ifdef DEBUG
            printf("[prepare_ctx] stdout: '%s'; append: %d\n", std_out_file->p, append);
        #endif
        ctx->std_out = calloc(1, sizeof(FIL));
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
    ctx->argv = (char**)calloc(sizeof(char*), lst->size);
    node_t* n = lst->first;
    for(size_t i = 0; i < lst->size && n != NULL; ++i, n = n->next) {
        ctx->argv[i] = copy_str(c_str(n->data));
    }
    delete_list(lst);
    if (ctx->orig_cmd) free(ctx->orig_cmd);
    ctx->orig_cmd = copy_str(ctx->argv[0]);
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
    #ifdef DEBUG
        printf("[cmd_enter] %s\n", s_cmd->p);
    #endif
    bool exit = false;
    bool in_qutas = false;
    cmd_ctx_t* ctxi = ctx;
    string_t* t = new_string_v();
    for (size_t i = 0; i <= s_cmd->size; ++i) {
        char c = s_cmd->p[i];
        if (!c) {
            #ifdef DEBUG
                printf("[cmd_enter] [%s] pass to [prepare_ctx]\n", t->p);
            #endif
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
            curr->std_out = calloc(1, sizeof(FIL));
            curr->std_err = curr->std_out;
            next->std_in = calloc(1, sizeof(FIL));
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
        s_b = new_string_v();
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
            if (c == CHAR_CODE_BS) cmd_backspace();
            else if (c == CHAR_CODE_UP) cmd_up(ctx);
            else if (c == CHAR_CODE_DOWN) cmd_down(ctx);
            else if (c == CHAR_CODE_TAB) cmd_tab(ctx);
            else if (c == CHAR_CODE_ESC) {
                blimp(15, 1);
                printf("\n");
                string_resize(s_cmd, 0);
                prompt(ctx);
            }
            else if (c == CHAR_CODE_ENTER) {
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
