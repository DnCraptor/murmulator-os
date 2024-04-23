typedef void (*vApplicationMallocFailedHookPtr)( void );

vApplicationMallocFailedHookPtr getApplicationMallocFailedHookPtr();
void setApplicationMallocFailedHookPtr(vApplicationMallocFailedHookPtr ptr);

typedef void (*vApplicationStackOverflowHookPtr)( TaskHandle_t pxTask, char *pcTaskName );

vApplicationStackOverflowHookPtr getApplicationStackOverflowHookPtr();
void setApplicationStackOverflowHookPtr(vApplicationStackOverflowHookPtr ptr);
