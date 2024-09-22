#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "cmd.h"

#define FIRMWARE_MARKER_FN "/.firmware"

void reboot_me(void);
bool load_firmware(char* pathname);
void run_app(char * name);
void link_firmware(FIL* pf, const char* pathname);

bool is_new_app(cmd_ctx_t* ctx);
bool run_new_app(cmd_ctx_t* ctx);

bool load_app(cmd_ctx_t* ctx);
void exec(cmd_ctx_t* ctx);
void cleanup_bootb_ctx(cmd_ctx_t* ctx);
void flash_block(uint8_t* buffer, size_t flash_target_offset);
size_t free_app_flash(void);

void mallocFailedHandler();
void overflowHook( TaskHandle_t pxTask, char *pcTaskName );
void vCmdTask(void *pv);
void app_signal(void);
int kill(uint32_t task_number);

#ifdef __cplusplus
}
#endif
