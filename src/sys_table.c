#include "FreeRTOS.h"
#include "task.h"
#include "sys_table.h"
#include "graphics.h"
#include "ff.h"
#include "hooks.h"

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
    // graphics.h
    draw_text, // 27
    draw_window, // 
    // TODO:
    0
};
