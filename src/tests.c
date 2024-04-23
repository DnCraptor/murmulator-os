#include "m-os-api.h"

void vTask1(void *pv) {
    char string[64] = {0};
    for(unsigned long i = 0; ; ++i) {
        snprintf(string, 64, pv, i);
        draw_text(string, 0, 1, 13, 1);
        vTaskDelay(25 * portTICK_PERIOD_MS);
    }
}

void vTask2(void *pv) {
    char string[64] = {0};
    for(unsigned long i = 0; ; ++i) {
        snprintf(string, 64, pv, i);
        draw_text(string, 0, 2, 13, 1);
        vTaskDelay(25 * portTICK_PERIOD_MS);
    }
}

void start_test() {
    draw_text("Not RUN", 0, 3, 13, 1);
    BaseType_t
    // test direct creation
    res = xTaskCreate(vTask1, "Task 1", configMINIMAL_STACK_SIZE, "vTaks 1 #%d", configMAX_PRIORITIES - 1, NULL);
    if (res != pdPASS) {
        draw_text("vTask1 failed to schedule", 0, 1, 13, 1);
    }
    res = xTaskCreate(vTask2, "Task 2", configMINIMAL_STACK_SIZE, "vTask 2 #%d", configMAX_PRIORITIES - 1, NULL);
    if (res != pdPASS) {
        draw_text("vTask2 failed to schedule", 0, 2, 13, 1);
    }
    TaskHandle_t hndl;
    res = xTaskCreate(vTask2, "Task 3", configMINIMAL_STACK_SIZE, "vTask 3 #%d", configMAX_PRIORITIES - 1, &hndl);
    if (res != pdPASS) {
        draw_text("vTask2(3) failed to schedule", 0, 2, 13, 1);
    }
    vTaskDelete(hndl);
    draw_text("RUN    ", 0, 3, 13, 1);
}
