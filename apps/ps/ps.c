#include "m-os-api.h"

int main() {
//    cmd_ctx_t* ctx = get_cmd_ctx();
    configRUN_TIME_COUNTER_TYPE ulTotalRunTime, ulStatsAsPercentage;
    volatile UBaseType_t uxArraySize = uxTaskGetNumberOfTasks();
    TaskStatus_t *pxTaskStatusArray = pvPortMalloc( uxArraySize * sizeof( TaskStatus_t ) );
    uxArraySize = uxTaskGetSystemState( pxTaskStatusArray, uxArraySize, &ulTotalRunTime );
    for ( UBaseType_t x = 0; x < uxArraySize; x++ ) {
        printf("[%d] %s\n", pxTaskStatusArray[ x ].xTaskNumber, pxTaskStatusArray[ x ].pcTaskName);
    }
    vPortFree( pxTaskStatusArray );
    return 0;
}

int __required_m_api_verion(void) {
    return M_API_VERSION;
}
