#ifndef CMD_H
#define CMD_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include "ff.h"
#include "FreeRTOS.h"
#include "task.h"
#include "../../api/m-os-api-c-list.h"

size_t get_heap_total();

char* get_curr_dir(); // old system
FIL * get_stdout(); // old system
FIL * get_stderr(); // old system


typedef struct {
    char* del_addr;
    char* prg_addr;
    uint16_t sec_num;
} sect_entry_t;

static void sect_entry_deallocator(sect_entry_t* s) {
    if (s->del_addr) vPortFree(s->del_addr);
    vPortFree(s);
}

typedef int (*bootb_ptr_t)( void );

typedef struct {
    bootb_ptr_t bootb[5];
    list_t* /*sect_entry_t*/ sections;
} bootb_ctx_t;

typedef struct {
    const char* key;
    char* value;
} vars_t;

typedef enum {
    INITIAL,
    PREPARED,
    FOUND,
    VALID,
    LOAD,
    EXECUTED,
    INVALIDATED,
    SIGTERM
} cmd_exec_stage_t;

typedef struct cmd_ctx {
    uint32_t argc;
    char** argv;
    char* orig_cmd;

    FIL* std_in;
    FIL* std_out;
    FIL* std_err;
    int ret_code;
    bool detached;

    struct cmd_ctx* prev;
    bootb_ctx_t* pboot_ctx;

    vars_t* vars;
    size_t vars_num;

    struct cmd_ctx* next;

    volatile cmd_exec_stage_t stage;
    void* user_data;
    bool forse_flash;
} cmd_ctx_t;

cmd_ctx_t* get_cmd_startup_ctx(); // system
cmd_ctx_t* get_cmd_ctx();
cmd_ctx_t* clone_ctx(cmd_ctx_t* src);
void remove_ctx(cmd_ctx_t*);
void set_ctx_var(cmd_ctx_t*, const char* key, const char* val);
char* get_ctx_var(cmd_ctx_t*, const char* key);
void cleanup_ctx(cmd_ctx_t* src);

char* next_token(char* t);
char* concat(const char* s1, const char* s2);
bool exists(cmd_ctx_t* ctx);
char* concat2(const char* s1, size_t sz, const char* s2);
char* copy_str(const char* s);
void show_logo(bool with_top);


// helpers for `cmd` and `mc` to reduce their size
#include "FreeRTOS.h"
#include "task.h"
#include "../../api/m-os-api-c-string.h"

void cmd_tab(cmd_ctx_t* ctx, string_t* s_cmd);
int history_steps(cmd_ctx_t* ctx, int cmd_history_idx, string_t* s_cmd);
bool cmd_enter_helper(cmd_ctx_t* ctx, string_t* s_cmd);

typedef FRESULT (*FRFpvUpU_ptr_t)(FIL*, void*, UINT, UINT*);
void op_console(cmd_ctx_t* ctx, FRFpvUpU_ptr_t fn, BYTE mode);
bool f_read_str(FIL* f, char* buf, size_t lim);

#ifdef __cplusplus
}
#endif

#endif // CMD_H
