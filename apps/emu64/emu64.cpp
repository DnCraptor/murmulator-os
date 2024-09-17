#include "m-os-api.h"
#include "m-os-api-cpp-psram.h"
#include "m-os-api-math.h"
// TODO:
#undef switch

#include "c64_class.h"
#define VERSION_STRING "1"
#define LogText(x)
#define nullptr 0


static void mem_stat(char* s)
{
    printf("%s\n",s);
    HeapStats_t stat;
    vPortGetHeapStats(&stat);
    size_t heap = get_heap_total();
    printf( "Heap memory: %d (%dK)\n"
            " available bytes total: %d (%dK)\n"
            "         largets block: %d (%dK)\n",
            heap, heap >> 10,
            stat.xAvailableHeapSpaceInBytes, stat.xAvailableHeapSpaceInBytes >> 10,
            stat.xSizeOfLargestFreeBlockInBytes, stat.xSizeOfLargestFreeBlockInBytes >> 10
    );
    printf( "        smallest block: %d (%dK)\n"
            "           free blocks: %d\n"
            "    min free remaining: %d (%dK)\n"
            "           allocations: %d\n"
            "                 frees: %d\n",
            stat.xSizeOfSmallestFreeBlockInBytes, stat.xSizeOfSmallestFreeBlockInBytes >> 10,
            stat.xNumberOfFreeBlocks,
            stat.xMinimumEverFreeBytesRemaining, stat.xMinimumEverFreeBytesRemaining >> 10,
            stat.xNumberOfSuccessfulAllocations, stat.xNumberOfSuccessfulFrees
    );
}

/// TODO:
int main(void) {
    printf("main %d\n", sizeof(C64Class));
    mem_stat("!");
    int ret_error;
    C64Class с64(&ret_error, 1024, "/c64");
    printf("c64\n");
    /// ??
    return ret_error;
}

#include "c64_class.cpp"
#include "mmu_class.cpp"
#include "cartridge_class.cpp"
#include "mos6510_class.cpp"
#include "mos6569_class.cpp"
#include "mos6526_port.cpp"
#include "mos6581_8085_class.cpp"
#include "mos6526_class.cpp"
#include "reu_class.cpp"
#include "georam_class.cpp"
#include "tape1530_class.cpp"
#include "floppy1541_class.cpp"
#include "am29f040_class.cpp"
#include "mos6502_class.cpp"
#include "mos6522_class.cpp"
