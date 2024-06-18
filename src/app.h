#pragma once
#include <stdbool.h>
#include <stdint.h>

bool load_firmware(const char pathname[256]);
void run_app(char * name);

bool is_new_app(char * name);
int run_new_app(char * name, char * fn);

typedef struct {
    char* sec_addr;
    uint16_t sec_num;
} sect_entry_t;

typedef int (*bootb_ptr_t)( void );

int load_app(char * fn, char * fn1, bootb_ptr_t bootb_tbl[4], sect_entry_t** sects_list);
int exec(sect_entry_t* sects_list, bootb_ptr_t bootb_tbl[4]);
