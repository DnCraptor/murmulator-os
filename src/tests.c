#include "FreeRTOS.h"
#include "task.h"
#include "graphics.h"
#include "m-os-api.h"

void vTask1(void *pv) {
    draw_text("vTask1", 0, 1, 13, 1);
    for(;;) {
        draw_text("vTask1 cycle", 0, 1, 13, 1);
        vTaskDelay(25 * portTICK_PERIOD_MS);
    }
}

void vTask2(void *pv) {
    draw_text("vTask2", 0, 2, 13, 1);
    for(unsigned long i = 0; ; ++i) {
        char string[TEXTMODE_COLS + 1] = {0};
        snprintf(string, TEXTMODE_COLS, "vTask2 cycle #%d", i);
        draw_text(string, 0, 2, 13, 1);
        vTaskDelay(25 * portTICK_PERIOD_MS);
    }
}

void start_test() {
    void (*draw_text_ptr)(const char string[TEXTMODE_COLS + 1], uint32_t x, uint32_t y, uint8_t color, uint8_t bgcolor)
    = (void*)_sys_table_ptrs[_draw_text_ptr_idx];

    draw_text_ptr("Not RUN", 0, 3, 13, 1);
    BaseType_t
    // test direct creation
    res = xTaskCreate(vTask1, "Task 1", configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES - 1, NULL);
    if (res != pdPASS) {
        draw_text("vTask1 failed to schedule", 0, 1, 13, 1);
    }
    BaseType_t (*xTaskCreatePtr)( TaskFunction_t pxTaskCode,
                            const char * const pcName,
                            const configSTACK_DEPTH_TYPE uxStackDepth,
                            void * const pvParameters,
                            UBaseType_t uxPriority,
                            TaskHandle_t * const pxCreatedTask ) = _sys_table_ptrs[0];
    res = xTaskCreatePtr(vTask2, "Task 2", configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES - 1, NULL);
    if (res != pdPASS) {
        draw_text("vTask2 failed to schedule", 0, 2, 13, 1);
    }
    draw_text("RUN    ", 0, 3, 13, 1);
}
