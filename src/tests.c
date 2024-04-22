#include "m-os-api.h"

void vTask1(void *pv) {
    char string[64] = {0};
    for(unsigned long i = 0; ; ++i) {
        snprintf(string, 64, pv, i);
        draw_text_m(string, 0, 1, 13, 1);
        vTaskDelayM(25 * portTICK_PERIOD_MS);
    }
}

void vTask2(void *pv) {
    char string[64] = {0};
    for(unsigned long i = 0; ; ++i) {
        snprintf(string, 64, pv, i);
        draw_text_m(string, 0, 2, 13, 1);
        vTaskDelayM(25 * portTICK_PERIOD_MS);
    }
}

void start_test() {
    draw_text_m("Not RUN", 0, 3, 13, 1);
    BaseType_t
    // test direct creation
    res = xTaskCreateM(vTask1, "Task 1", configMINIMAL_STACK_SIZE, "vTaks 1 #%d", configMAX_PRIORITIES - 1, NULL);
    if (res != pdPASS) {
        draw_text_m("vTask1 failed to schedule", 0, 1, 13, 1);
    }
    res = xTaskCreateM(vTask2, "Task 2", configMINIMAL_STACK_SIZE, "vTask 2 #%d", configMAX_PRIORITIES - 1, NULL);
    if (res != pdPASS) {
        draw_text_m("vTask2 failed to schedule", 0, 2, 13, 1);
    }
    draw_text_m("RUN    ", 0, 3, 13, 1);
}
