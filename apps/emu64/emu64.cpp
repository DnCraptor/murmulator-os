#include "m-os-api.h"
#include "m-os-api-cpp-psram.h"

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