#include "m-os-api.h"

int main() {
    cmd_ctx_t* ctx = get_cmd_ctx();
    int idx = 1;
    if (ctx->argc == 1) {
        fgoutf(ctx->std_err, "Unable to kill processs with not specified number\n");
        return 1;
    }
    if (ctx->argc > 2) {
        if ( 0 == strcmp(ctx->argv[1], "-9") ) {
            if (ctx->argc > 3) goto e1;
            idx = 2;
        } else {
            e1:
            fgoutf(ctx->std_err, "Unable to kill more than one process\n");
            return 1;
        }
    }
    uint32_t ps_num = atoi(ctx->argv[idx]);
    int res;
    if (idx == 2) { // kill -9
        configRUN_TIME_COUNTER_TYPE ulTotalRunTime, ulStatsAsPercentage;
        volatile UBaseType_t uxArraySize = uxTaskGetNumberOfTasks();
        TaskStatus_t *pxTaskStatusArray = pvPortMalloc( uxArraySize * sizeof( TaskStatus_t ) );
        uxArraySize = uxTaskGetSystemState( pxTaskStatusArray, uxArraySize, &ulTotalRunTime );
        int res = 0;
        for ( UBaseType_t x = 0; x < uxArraySize; x++ ) {
            if (pxTaskStatusArray[ x ].xTaskNumber == ps_num) {
                res = 1;
                cmd_ctx_t* ctx = (cmd_ctx_t*) pvTaskGetThreadLocalStoragePointer(pxTaskStatusArray[ x ].xHandle, 0);
                if (ctx) {
                    ctx->stage = SIGTERM;
                    if (ctx->pboot_ctx && ctx->pboot_ctx->bootb[4]) {
                        ctx->pboot_ctx->bootb[4](); // signal SIGTERM
                        res = 2;
                    }
                    vTaskDelete(pxTaskStatusArray[ x ].xHandle);
                    cleanup_ctx(ctx);
                } else {
                    res = 3;
                }
                break;
            }
        }
        vPortFree( pxTaskStatusArray );
    } else {
        res = kill(ps_num);
    }
    if (res == 0) {
        fprintf(ctx->std_err, "Task #%d not found.\n", ps_num);
    }
    else if (res == 1) {
        fprintf(ctx->std_err, "Task #%d has no SIGTERM action support (process state was set to SIGTERM).\n", ps_num);
    }
    else if (res == 2) {
        printf("Task #%d - SIGTERM was sent.\n", ps_num);
    }
    else if (res == 3) {
        printf("Task #%d - cmd context is not defined.\n", ps_num);
    }
    return 0;
}
