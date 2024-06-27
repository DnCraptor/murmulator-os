#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include "ff.h"

size_t get_heap_total();

char* get_curr_dir(); // old system
FIL * get_stdout(); // old system
FIL * get_stderr(); // old system


typedef struct {
    char* del_addr;
    char* prg_addr;
    uint16_t sec_num;
} sect_entry_t;

typedef int (*bootb_ptr_t)( void );

typedef struct {
    bootb_ptr_t bootb[4];
    sect_entry_t* sect_entries;
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
    INVALIDATED
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

    char* curr_dir;
    bootb_ctx_t* pboot_ctx;

    vars_t* vars;
    size_t vars_num;

    struct cmd_ctx* pipe;

    cmd_exec_stage_t stage;
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

#ifdef __cplusplus
}
#endif
