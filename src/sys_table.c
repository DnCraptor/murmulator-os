#include <time.h>
#include <hardware/flash.h>
#include <pico/multicore.h>
#include <pico/stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "sys_table.h"
#include "graphics.h"
#include "ff.h"
#include "hooks.h"
#include "portable.h"
#include "timers.h" // TODO
#include "ps2.h"
#include "app.h"
#include "cmd.h"
#include "psram_spi.h"
#include "math-wrapper.h"
#include "ram_page.h"
#include "overclock.h"
#include "hardfault.h"
#include "keyboard.h"
#include "usb.h"
#include "nespad.h"
#include "sound.h"

FATFS* get_mount_fs(); // only one FS is supported foe now

// to cleanup BOOTA memory region on the MOS flashing
unsigned long __in_boota() __aligned(4096) cleanup_boota[] = { 0 };

unsigned long __in_systable() __aligned(4096) sys_table_ptrs[] = {
    // task.h
    xTaskCreate, // 0
    vTaskDelete, // 1
    vTaskDelay,  // 2
    xTaskDelayUntil, // 3
    xTaskAbortDelay, // 4
    uxTaskPriorityGet, // 5
    uxTaskPriorityGetFromISR, // 6
    uxTaskBasePriorityGet, // 7
    uxTaskBasePriorityGetFromISR, // 8
    eTaskGetState, // 9
    vTaskGetInfo, // 10
    vTaskPrioritySet, // 11
    vTaskSuspend, // 12
    vTaskResume, // 13
    xTaskResumeFromISR, // 14
    vTaskSuspendAll, // 15
    xTaskResumeAll, // 16
    xTaskGetTickCount, // 17
    xTaskGetTickCountFromISR, // 18
    uxTaskGetNumberOfTasks, // 19
    pcTaskGetName, // 20
    xTaskGetHandle, // 21
    uxTaskGetStackHighWaterMark, // 22
    vTaskSetThreadLocalStoragePointer, // 23
    pvTaskGetThreadLocalStoragePointer, // 24
    // TODO: hooks support?
    getApplicationMallocFailedHookPtr, // 25
    setApplicationMallocFailedHookPtr, // 26
    getApplicationStackOverflowHookPtr, // 27
    setApplicationStackOverflowHookPtr, // 28
    // #include <stdio.h>
    snprintf, // 29
    // graphics.h
    draw_text, // 30 
    draw_window, // 31
    // 
    pvPortMalloc, // 32
    vPortFree, // 33
    //
    graphics_set_mode, // 34
    graphics_lock_buffer, // was graphics_set_buffer 35
    graphics_set_offset, // 36
    0, // graphics_set_palette, // 37
    graphics_set_buffer, // 38
    graphics_set_bgcolor, // 39
    0, // graphics_set_flashmode, // 40
    goutf, // 41
    graphics_set_con_pos, // 42
    graphics_set_con_color, // 43
    clrScr, // 44
    gbackspace, // 45

    f_open, // 46
    f_close, // 47
    f_write, // 48
    f_read, // 49
    f_stat, // 50
    f_lseek, // 51
    f_truncate, // 52
    f_sync, // 53
    f_opendir, // 54
    f_closedir, // 55
    f_readdir, // 56
    f_mkdir, // 57
    f_unlink, // 58
    f_rename, // 59
    strcpy, // 60
    f_getfree, // 61
    //
    strlen, // 62
    strncpy, // 63
    get_leds_stat, // 64
    //
    load_firmware, // 65
    run_app, // 66
    //
    vsnprintf, // 67
    //
    get_stdout, // 68
    get_stderr, // 69
    //
    fgoutf, // 70
    //
    psram_size, // 71
    psram_cleanup, // 72
    write8psram, // 73
    write16psram, // 74
    write32psram, // 75
    read8psram, // 76
    read16psram, // 77
    read32psram, // 78
    //
    __u32u32u32_div, // 79
    __u32u32u32_rem, // 80
    __fff_div, // 81
    __fff_mul, // 82
    __ffu32_mul, // 83
    __ddd_div, // 84
    __ddd_mul, // 85
    __ddu32_mul, // 86
    __ddf_mul, // 87
    __ffu32_div, // 88
    __ddu32_div, // 89
    //
    swap_size, // 90
    swap_base_size, // 91
    swap_base, // 92
    ram_page_read, // 93
    ram_page_read16, // 94
    ram_page_read32, // 95
    ram_page_write, // 96
    ram_page_write16, // 97
    ram_page_write32, // 98
    //
    get_cmd_startup_ctx, // 99
    //
    atoi, // 100
    //
    overclocking, // 101
    overclocking_ex, // 102
    get_overclocking_khz, // 103
    set_overclocking, // 104
    set_sys_clock_pll, // 105
    check_sys_clock_khz, // 106
    //
    next_token, // 107
    //
    strcmp, // 108
    strncmp, // 109
    //
    vPortGetHeapStats, // 110
    get_cpu_ram_size, // 111
    get_cpu_flash_size, // 112
    //
    get_mount_fs, // 113
    f_getfree32, // 114
    //
    get_scancode_handler, // 115
    set_scancode_handler, // 116
    get_cp866_handler, // 117
    set_cp866_handler, // 118
    gbackspace, // 119
    //
    is_new_app, // 120
    run_new_app, // 121
    //
    __getch, // 122
    __putc, // 123
    //
    cleanup_bootb_ctx, // 124
    load_app, // 125
    exec, // 126
    //
    gouta, // 127
    //
    exists, // 128
    concat, // 129
    concat2, // 130
    //
    flash_block_wrapper, // 131
    get_heap_total, // 132
    swap_pages, // 133
    swap_pages_base, // 134
    swap_page_size, // 135
    //
    xTaskGetCurrentTaskHandle, // 136
    copy_str, // 137
    get_cmd_ctx, // 138
    cleanup_ctx, // 139
    get_ctx_var, // 140
    set_ctx_var, // 141
    memset, // 142
    clone_ctx, // 143
    remove_ctx, // 144
    //
    xPortGetFreeHeapSize, // 145
    //
    get_console_width, // 146
    get_console_height, // 147
    //
    __getc, // 148
    f_eof, // 149
    f_getc, // 150
    f_open_pipe, // 151
    //
    get_buffer, // 152
    get_buffer_size, // 153
    get_screen_bitness, // 154
    cleanup_graphics, // 155
    install_graphics_driver, // 156
    get_console_bitness, // 157
    get_screen_width, // 158
    get_screen_height, // 159
    get_graphics_driver, // 160
    is_buffer_text, // 161
    graphics_get_mode, // 162
    graphics_is_mode_text, // 163
    set_vga_dma_handler_impl, // 164 (TODO: organize)
    set_vga_clkdiv, // 165
    pvPortCalloc, // 166
    memcpy, // 167
    vga_dma_channel_set_read_addr, // 168
    //
    qsort, // 169
    strnlen,  // 170
    flash_do_cmd, // 171
    flash_range_erase, // 172
    flash_range_program, // 173
    flash_get_unique_id, // 174
    multicore_lockout_start_blocking, // 175
    multicore_lockout_end_blocking, // 176
    get_cpu_flash_jedec_id, // 177
    psram_id, // 178
    //
    init_pico_usb_drive, // 179 (better use usb_driver)
    pico_usb_drive_heartbeat, // 180 (better use usb_driver)
    tud_msc_ejected, // 181
    set_tud_msc_ejected, // 182 (better use usb_driver)
    //
    show_logo, // 183
    getch_now, // 184
    //
    usb_driver, // 185
    set_cursor_color, // 186
    //
    nespad_stat, // 187
    graphics_get_default_mode, // 188
    //
    graphics_get_font_table, // 189
    graphics_get_font_width, // 190
    graphics_get_font_height, // 191
    graphics_set_font, // 192
    graphics_set_ext_font, // 193
    //
    blimp, // 194
    graphics_con_x, // 195
    graphics_con_y, // 196
    //
    pcm_setup, // 197
    pcm_cleanup, // 198
    pcm_set_buffer, // 199
    // v.0.2.4
    __trunc, // 200
    __floor, // 201
    __pow, // 202
    __sqrt, // 203
    __sin, // 204
    __cos, // 205
    __tan, // 206
    __atan, // 207
    __log, // 208
    __exp, // 109
    // TODO:
    0
};
