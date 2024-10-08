#include "m-os-api.h"
#include <hardware/timer.h>

volatile bool marked_to_exit;

int main(void) {
    marked_to_exit = false;
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
    if (marked_to_exit) return 0;

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
    if (marked_to_exit) return 0;

    begin = time_us_32();
    for (a = 0; a < sz; a += 2) {
        *(uint16_t*)(p + a) = a & 0xFFFF;
    }
    elapsed = time_us_32() - begin;
    speed = __ddu32_div(__ddu32_mul(d, a), elapsed);
    fgoutf(f, "16-bit line write speed: %f MBps\n", speed);
    if (marked_to_exit) return 0;

    begin = time_us_32();
    for (a = 0; a < sz; a += 2) {
        if ((a & 0xFFFF) != *(uint16_t*)(p + a)) {
            fgoutf(f, "16-bit read failed at %ph\n", a);
            break;
        }
    }
    elapsed = time_us_32() - begin;
    speed = __ddu32_div(__ddu32_mul(d, a), elapsed);
    fgoutf(f, "16-bit line read speed: %f MBps\n", speed);
    if (marked_to_exit) return 0;

    begin = time_us_32();
    for (a = 0; a < sz; a += 4) {
        *(uint32_t*)(p + a) = a;
    }
    elapsed = time_us_32() - begin;
    speed = __ddu32_div(__ddu32_mul(d, a), elapsed);
    fgoutf(f, "32-bit line write speed: %f MBps\n", speed);
    if (marked_to_exit) return 0;

    begin = time_us_32();
    for (a = 0; a < sz; a += 4) {
        if (a != *(uint32_t*)(p + a)) {
            fgoutf(f, "32-bit read failed at %ph\n", a);
            break;
        }
    }
    elapsed = time_us_32() - begin;
    speed = __ddu32_div(__ddu32_mul(d, a), elapsed);
    fgoutf(f, "32-bit line read speed: %f MBps\n", speed);

    return 0;
}
