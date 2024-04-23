typedef void (*vApplicationMallocFailedHookPtr)( void );

vApplicationMallocFailedHookPtr getApplicationMallocFailedHookPtr();
void setApplicationMallocFailedHookPtr(vApplicationMallocFailedHookPtr ptr);
