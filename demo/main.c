#include "m-os-api.h"
#include "pico/stdlib.h"

#define T1_LINE 24
#define T2_LINE 25

void vTask1(void *pv) {
    draw_text("vTask1 running   ", 0, T1_LINE, 13, 1);
    char *string = pvPortMalloc(64);
    for(unsigned long i = 0; ; ++i) {
        gpio_put(PICO_DEFAULT_LED_PIN, 0);
        snprintf(string, 64, pv, i);
        draw_text(string, 0, T1_LINE, 13, 1);
        vTaskDelay(500 * portTICK_PERIOD_MS);
        gpio_put(PICO_DEFAULT_LED_PIN, 1);
        vTaskDelay(500 * portTICK_PERIOD_MS);
    }
    vPortFree(string);
}

void vTask2(void *pv) {
    draw_text("vTask2 running   ", 0, T2_LINE, 13, 1);
    static char string[64] = {0};
    for(unsigned long i = 0; ; ++i) {
        snprintf(string, 64, pv, i);
        draw_text(string, 0, T2_LINE, 13, 1);
        vTaskDelay(25 * portTICK_PERIOD_MS);
    }
}

static vApplicationMallocFailedHookPtr mh = NULL;

void mallocFailedHandler() {
    draw_text("vApplicationMallocFailedHook", 0, 6, 13, 1);
    if (mh) mh();
}

static vApplicationStackOverflowHookPtr soh = NULL;

void overflowHook( TaskHandle_t pxTask, char *pcTaskName ) {
    draw_text("vApplicationStackOverflowHook", 0, 7, 13, 1);
    if(soh) soh(pxTask, pcTaskName);
}

int boota(void* pv) {
    gpio_put(PICO_DEFAULT_LED_PIN, 1);
    draw_text("Not RUN", 0, 3, 13, 1);
    mh = getApplicationMallocFailedHookPtr();
    setApplicationMallocFailedHookPtr(mallocFailedHandler);
    soh = getApplicationStackOverflowHookPtr();
    setApplicationStackOverflowHookPtr(overflowHook);
    BaseType_t
    res = xTaskCreate(vTask1, "Task 1", configMINIMAL_STACK_SIZE, "vTaks 1 #%d", configMAX_PRIORITIES - 1, NULL);
    if (res != pdPASS) {
        draw_text("vTask1 failed to schedule", 0, T1_LINE + 1, 13, 1);
        return -1;
    }
    draw_text("vTask1 scheduled", 0, T1_LINE + 2, 7, 0);
    res = xTaskCreate(vTask2, "Task 2", configMINIMAL_STACK_SIZE, "vTask 2 #%d", configMAX_PRIORITIES - 1, NULL);
    if (res != pdPASS) {
        draw_text("vTask2 failed to schedule", 0, T2_LINE + 3, 13, 1);
        return -2;
    }
    draw_text("vTask2 scheduled", 0, T2_LINE, 7, 0);
    TaskHandle_t hndl;
    res = xTaskCreate(vTask2, "Task 3", configMINIMAL_STACK_SIZE, "vTask 3 #%d", configMAX_PRIORITIES - 1, &hndl);
    if (res != pdPASS) {
        draw_text("vTask2(3) failed to schedule", 0, T2_LINE + 4, 13, 1);
        return -3;
    }
    vTaskDelete(hndl);
    draw_text("RUN      ", 0, 3, 7, 0);
    return 0;
}

unsigned long __aligned(4096) __in_boota() boota_tbl[] = { boota };


// just for std linker
void main() {
    int r = boota(NULL);
    r = boota_tbl;
    (void*)r;
}
