#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

bool load_firmware(char* pathname);
void run_app(char * name);

bool is_new_app(char * name);
int run_new_app(char * name);

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

int load_app(char* name, bootb_ctx_t* bootb_ctx);
int exec(bootb_ctx_t* bootb_ctx);
void cleanup_bootb_ctx(bootb_ctx_t* bootb_ctx);
bool restore_tbl(char* fn);
void flash_block(uint8_t* buffer, size_t flash_target_offset);

#ifdef __cplusplus
}
#endif
