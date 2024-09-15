#include "m-os-api.h"
#include "m-os-api-cpp-psram.h"
#include "m-os-api-math.h"
// TODO:
#undef switch

#include "c64_class.h"
#define VERSION_STRING "1"
#define LogText(x)

/// TODO:
int main(void) {
    int ret_error;
    C64Class * c64 = new C64Class(&ret_error, 1024, "/c64");
    /// ??
    delete c64;
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
