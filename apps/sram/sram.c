#include "m-os-api.h"
#include <hardware/timer.h>

int main(void) {
    FIL * f = get_stdout();
    uint32_t sz = swap_base_size();
    uint8_t* p = swap_base();
    fgoutf(f, "SWAP BASE size: %d bytes @ %ph\n", sz, p);
    uint32_t a = 0;
    uint32_t begin = time_us_32();
    for (; a < sz; ++a) {
        p[a] = a & 0xFF;
    }
    uint32_t elapsed = time_us_32() - begin;
    double d = 1.0;
    double speed = __ddu32_div(__ddu32_mul(d, a), elapsed);
    fgoutf(f, "8-bit line write speed: %f MBps\n", speed);

    begin = time_us_32();
    for (a = 0; a < sz; ++a) {
        if ((a & 0xFF) != p[a]) {
            fgoutf(f, "8-bit read failed at %ph\n", a);
            break;
        }
    }
    elapsed = time_us_32() - begin;
    speed = __ddu32_div(__ddu32_mul(d, a), elapsed);
    fgoutf(f, "8-bit line read speed: %f MBps\n", speed);
    return 0;
}

int __required_m_api_verion(void) {
    return 2;
}
