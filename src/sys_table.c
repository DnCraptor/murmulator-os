#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "sys_table.h"
#include "graphics.h"
#include "ff.h"
#include "hooks.h"
#include "portable.h"
#include "timers.h" // TODO

// to cleanup BOOTA memory region on the MOS flashing
unsigned long __in_boota() __aligned(4096) cleanup_boota[] = { 0 };

unsigned long __in_systable() __aligned(4096) sys_table_ptrs[] = {
    // task.h
    xTaskCreate, // 0
    vTaskDelete, // 1
    vTaskDelay,  // 2
    xTaskDelayUntil, // 3
    xTaskAbortDelay, // 4
    uxTaskPriorityGet, // 5
    uxTaskPriorityGetFromISR, // 6
    uxTaskBasePriorityGet, // 7
    uxTaskBasePriorityGetFromISR, // 8
    eTaskGetState, // 9
    vTaskGetInfo, // 10
    vTaskPrioritySet, // 11
    vTaskSuspend, // 12
    vTaskResume, // 13
    xTaskResumeFromISR, // 14
    vTaskSuspendAll, // 15
    xTaskResumeAll, // 16
    xTaskGetTickCount, // 17
    xTaskGetTickCountFromISR, // 18
    uxTaskGetNumberOfTasks, // 19
    pcTaskGetName, // 20
    xTaskGetHandle, // 21
    uxTaskGetStackHighWaterMark, // 22
    vTaskSetThreadLocalStoragePointer, // 23
    pvTaskGetThreadLocalStoragePointer, // 24
    // TODO: hooks support?
    getApplicationMallocFailedHookPtr, // 25
    setApplicationMallocFailedHookPtr, // 26
    getApplicationStackOverflowHookPtr, // 27
    setApplicationStackOverflowHookPtr, // 28
    // #include <stdio.h>
    snprintf, // 29
    // graphics.h
    draw_text, // 30 
    draw_window, // 31
    pvPortMalloc, // 32
    vPortFree, // 33
    //
    graphics_set_mode, // 34
    graphics_set_buffer, // 35
    graphics_set_offset, // 36
    graphics_set_palette, // 37
    graphics_set_textbuffer, // 38
    graphics_set_bgcolor, // 39
    graphics_set_flashmode, // 40
    goutf, // 41
    graphics_set_con_pos, // 42
    graphics_set_con_color, // 43
    clrScr, // 44
    // TODO:
    0
};
