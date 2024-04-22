#define M_OS_API_SYA_TABLE_BASE (void*)0x10001000ul

#include "task.h"

static const unsigned long * const _sys_table_ptrs = M_OS_API_SYA_TABLE_BASE;

#define _xTaskCreatePtrIdx 0
#define _draw_text_ptr_idx 25
