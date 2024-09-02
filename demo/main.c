#define UF2_MODE

#include "m-os-api.h"
//#include "pico/stdlib.h"

#define T1_LINE 4
#define T2_LINE 5

typedef struct task_ctx {
    char* show_text;
} task_ctx_t;

void vTask1(void *pv) {
    cmd_ctx_t* ctx = (cmd_ctx_t*)pv;
    const TaskHandle_t th = xTaskGetCurrentTaskHandle();
    vTaskSetThreadLocalStoragePointer(th, 0, ctx);

    draw_text("vTask1 running   ", 0, T1_LINE, 13, 1);
    char *string = malloc(64);
    for(unsigned long i = 0; ctx->stage != SIGTERM; ++i) {
  //      gpio_put(PICO_DEFAULT_LED_PIN, 0);
        snprintf(string, 64, ((task_ctx_t*)ctx->user_data)->show_text, i);
        draw_text(string, 0, T1_LINE, 13, 1);
        vTaskDelay(500 * portTICK_PERIOD_MS);
    //    gpio_put(PICO_DEFAULT_LED_PIN, 1);
        vTaskDelay(500 * portTICK_PERIOD_MS);
    }
    free(string);
    free(ctx);
    vTaskDelete(NULL);
}

void vTask2(void *pv) {
    cmd_ctx_t* ctx = (cmd_ctx_t*)pv;
    const TaskHandle_t th = xTaskGetCurrentTaskHandle();
    vTaskSetThreadLocalStoragePointer(th, 0, ctx);

    draw_text("vTask2 running   ", 0, T2_LINE, 13, 1);
    static char string[64] = {0};
    for(unsigned long i = 0; ctx->stage != SIGTERM; ++i) {
        snprintf(string, 64, ((task_ctx_t*)ctx->user_data)->show_text, i);
        draw_text(string, 0, T2_LINE, 13, 1);
        vTaskDelay(25 * portTICK_PERIOD_MS);
    }
    free(ctx);
    vTaskDelete(NULL);
}

/* TODO: define a way howto
typedef struct app_ctx {
    vApplicationMallocFailedHookPtr mh;
    vApplicationStackOverflowHookPtr soh;
} app_ctx_t;


void mallocFailedHandler() {
    draw_text("vApplicationMallocFailedHook", 0, 6, 13, 1);
    if (mh) mh();
}

void overflowHook( TaskHandle_t pxTask, char *pcTaskName ) {
    draw_text("vApplicationStackOverflowHook", 0, 7, 13, 1);
    if(soh) soh(pxTask, pcTaskName);
}
*/

int boota(void* pv) {
   // gpio_put(PICO_DEFAULT_LED_PIN, 1);
    cmd_ctx_t* ctx1 = calloc(1, sizeof(cmd_ctx_t));
    cmd_ctx_t* ctx2 = calloc(1, sizeof(cmd_ctx_t));
    cmd_ctx_t* ctx3 = calloc(1, sizeof(cmd_ctx_t));
    ctx1->user_data = calloc(1, sizeof(task_ctx_t));
    ctx2->user_data = calloc(1, sizeof(task_ctx_t));
    ctx3->user_data = calloc(1, sizeof(task_ctx_t));

    draw_text("Not RUN", 0, 3, 13, 1);
    /*
    mh = getApplicationMallocFailedHookPtr();
    setApplicationMallocFailedHookPtr(mallocFailedHandler);
    soh = getApplicationStackOverflowHookPtr();
    setApplicationStackOverflowHookPtr(overflowHook);
    */
    ((task_ctx_t*)ctx1->user_data)->show_text = "vTaks 1 #%d        ";
    BaseType_t
    res = xTaskCreate(vTask1, "Task 1", configMINIMAL_STACK_SIZE, ctx1, configMAX_PRIORITIES - 1, NULL);
    if (res != pdPASS) {
        draw_text("vTask1 failed to schedule", 0, T1_LINE, 13, 1);
        return -1;
    }
    draw_text("vTask1 scheduled", 0, T1_LINE + 2, 7, 0);

    ((task_ctx_t*)ctx2->user_data)->show_text = "vTaks 2 #%d        ";
    res = xTaskCreate(vTask2, "Task 2", configMINIMAL_STACK_SIZE, ctx2, configMAX_PRIORITIES - 1, NULL);
    if (res != pdPASS) {
        draw_text("vTask2 failed to schedule", 0, T2_LINE, 13, 1);
        return -2;
    }
    draw_text("vTask2 scheduled", 0, T2_LINE, 7, 0);
    TaskHandle_t hndl;

    ((task_ctx_t*)ctx3->user_data)->show_text = "vTaks 3 #%d        ";
    res = xTaskCreate(vTask2, "Task 3", configMINIMAL_STACK_SIZE, ctx3, configMAX_PRIORITIES - 1, &hndl);
    if (res != pdPASS) {
        draw_text("vTask2(3) failed to schedule", 0, T2_LINE + 4, 13, 1);
        return -3;
    }
    vTaskDelete(hndl);
    draw_text("RUN      ", 0, 3, 7, 0);
    return 0;
}

unsigned long __aligned(4096) __in_boota() boota_tbl[] = { // mark pages used by program
    boota, // 2-nd page 10002000-10003000
    boota, // 3-nd page 10003000-10004000
    boota, // 4-nd page 10004000-10005000
    boota, // 5-nd page 10005000-10006000
    0
};

// just for std linker
void main() {
    int r = boota(NULL);
    r = boota_tbl;
    (void*)r;
}

// TODO: noruntime
void _exit(int status) {

}
