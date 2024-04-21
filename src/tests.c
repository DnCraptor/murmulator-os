#include "FreeRTOS.h"
#include "task.h"
#include "vga.h"

void vTask1(void *pv) {
    draw_text("vTask1", 0, 1, 13, 1);
    for(;;) {
        draw_text("vTask1 cycle", 0, 1, 13, 1);
        vTaskDelay(25 * portTICK_PERIOD_MS);
    }
}

void vTask2(void *pv) {
    draw_text("vTask2", 0, 2, 13, 1);
    for(;;) {
        draw_text("vTask2 cycle", 0, 2, 13, 1);
        vTaskDelay(25 * portTICK_PERIOD_MS);
    }
}

void start_test() {
    draw_text("Not RUN", 0, 3, 13, 1);
    BaseType_t
    res = xTaskCreate(vTask1, "Task 1", configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES - 1, NULL);
    if (res != pdPASS) {
        draw_text("vTask1 failed to schedule", 0, 1, 13, 1);
    }
    res = xTaskCreate(vTask2, "Task 2", configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES - 1, NULL);
    if (res != pdPASS) {
        draw_text("vTask2 failed to schedule", 0, 2, 13, 1);
    }
    draw_text("RUN    ", 0, 3, 13, 1);
}
