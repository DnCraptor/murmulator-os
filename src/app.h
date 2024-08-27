#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "cmd.h"

#define FIRMWARE_MARKER_FN "/.firmware"

bool load_firmware(char* pathname);
void run_app(char * name);

bool is_new_app(cmd_ctx_t* ctx);
bool run_new_app(cmd_ctx_t* ctx);

bool load_app(cmd_ctx_t* ctx);
void exec(cmd_ctx_t* ctx);
void cleanup_bootb_ctx(cmd_ctx_t* ctx);
void flash_block_wrapper(uint8_t* buffer, size_t flash_target_offset);

void mallocFailedHandler();
void overflowHook( TaskHandle_t pxTask, char *pcTaskName );
void vCmdTask(void *pv);
void app_signal(void);
int kill(uint32_t task_number);

#ifdef __cplusplus
}
#endif
