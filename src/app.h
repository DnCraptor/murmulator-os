#pragma once
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

bool load_firmware(const char pathname[256]);
void run_app(char * name);

bool is_new_app(char * name);
int run_new_app(char * name, char * fn);

typedef struct {
    char* sec_addr;
    uint16_t sec_num;
} sect_entry_t;

typedef int (*bootb_ptr_t)( void );

typedef struct {
    bootb_ptr_t bootb[4];
    sect_entry_t* sect_entries;
} bootb_ctx_t;

int load_app(char * fn, char * fn1, bootb_ctx_t* bootb_ctx);
int exec(bootb_ctx_t* bootb_ctx);
void cleanup_bootb_ctx(bootb_ctx_t* bootb_ctx);

#ifdef __cplusplus
}
#endif
