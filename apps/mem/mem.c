#include "m-os-api.h"

int main(void) {
    cmd_ctx_t* ctx = get_cmd_ctx();
    vTaskSuspendAll();
    uint32_t flash32 = get_cpu_flash_size();
    uint32_t ram32 = get_cpu_ram_size();
    uint32_t sz = psram_size();
    size_t free_sz = xPortGetFreeHeapSize();
    xTaskResumeAll();
    fprintf(ctx->std_out,
            "SRAM size : %d (%dK) bytes (%dK free)\n"
            "FLASH size: %d (%dK) bytes (%dK free)\n"
            "PSRAM size: %d (%dK) bytes\n", // TODO: <-- psram table
            ram32, ram32 >> 10, free_sz >> 10,
            flash32, flash32 >> 10, (flash32 >> 10) - 96, // TODO: <-- flash_table
            sz, sz >> 10);
    sz = swap_size();
    fprintf(ctx->std_out,
            "SWAP size : %d (%dK) bytes\n", sz, sz >> 10);
    sz = swap_base_size();
    fprintf(ctx->std_out,
            "SWAP SRAM size: %d (%dK) bytes\n", sz, sz >> 10);
    size_t vsz = get_buffer_size();
    fprintf(ctx->std_out, "VRAM in heap  : %d (%dK); video mode: %d x %d x %d bit\n",
            vsz, vsz >> 10,
            get_screen_width(), get_screen_height(), get_screen_bitness()
    );
    HeapStats_t* stat = malloc(sizeof(HeapStats_t));
    vPortGetHeapStats(stat);
    size_t heap = get_heap_total();
    fprintf(ctx->std_out,
            "Heap memory: %d (%dK)\n"
            " available bytes total: %d (%dK)\n"
            "         largets block: %d (%dK)\n",
            heap, heap >> 10,
            stat->xAvailableHeapSpaceInBytes, stat->xAvailableHeapSpaceInBytes >> 10,
            stat->xSizeOfLargestFreeBlockInBytes, stat->xSizeOfLargestFreeBlockInBytes >> 10
    );
    fprintf(ctx->std_out,
            "        smallest block: %d (%dK)\n"
            "           free blocks: %d\n"
            "    min free remaining: %d (%dK)\n"
            "           allocations: %d\n"
            "                 frees: %d\n",
            stat->xSizeOfSmallestFreeBlockInBytes, stat->xSizeOfSmallestFreeBlockInBytes >> 10,
            stat->xNumberOfFreeBlocks,
            stat->xMinimumEverFreeBytesRemaining, stat->xMinimumEverFreeBytesRemaining >> 10,
            stat->xNumberOfSuccessfulAllocations, stat->xNumberOfSuccessfulFrees
    );
    free(stat);
    return 0;
}

int __required_m_api_verion(void) {
    return M_API_VERSION;
}
