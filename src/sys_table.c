#include "FreeRTOS.h"
#include "task.h"
#include "sys_table.h"
#include "graphics.h"

unsigned long __in_systable() __aligned(4096) sys_table_ptrs[] = {
    xTaskCreate,
    draw_text,
    draw_window,
    0
};
