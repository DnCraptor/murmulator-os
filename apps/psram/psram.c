#include "m-os-api.h"
#include "m-os-api-timer.h"

volatile bool marked_to_exit;

int main(void) {
    marked_to_exit = false;
    FIL * f = get_stdout();
    uint32_t sz = psram_size();
    fgoutf(f, "PSRAM size: %d bytes\n", sz);
    if (marked_to_exit) return 0;

    uint8_t rx8[8];
    psram_id(rx8);
    fgoutf(f, "MF ID: %02x; KGD: %02x; EID: %02x%02x-%02x%02x-%02x%02x\n", rx8[0], rx8[1], rx8[2], rx8[3], rx8[4], rx8[5], rx8[6], rx8[7]);
    if (marked_to_exit) return 0;

    uint32_t a = 0;
    uint32_t begin = time_us_32();
    for (; a < sz; ++a) {
        write8psram(a, a & 0xFF);
    }
    uint32_t elapsed = time_us_32() - begin;
    double d = 1.0;
    double speed = __ddu32_div(__ddu32_mul(d, a), elapsed);
    fgoutf(f, "8-bit line write speed: %f MBps\n", speed);
    if (marked_to_exit) return 0;
    begin = time_us_32();
    for (a = 0; a < sz; ++a) {
        if ((a & 0xFF) != read8psram(a)) {
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
        write16psram(a, a & 0xFFFF);
    }
    elapsed = time_us_32() - begin;
    speed = __ddu32_div(__ddu32_mul(d, a), elapsed);
    fgoutf(f, "16-bit line write speed: %f MBps\n", speed);
    if (marked_to_exit) return 0;

    begin = time_us_32();
    for (a = 0; a < sz; a += 2) {
        if ((a & 0xFFFF) != read16psram(a)) {
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
        write32psram(a, a);
    }
    elapsed = time_us_32() - begin;
    speed = __ddu32_div(__ddu32_mul(d, a), elapsed);
    fgoutf(f, "32-bit line write speed: %f MBps\n", speed);
    if (marked_to_exit) return 0;

    begin = time_us_32();
    for (a = 0; a < sz; a += 4) {
        if (a != read32psram(a)) {
            fgoutf(f, "32-bit read failed at %ph\n", a);
            break;
        }
    }
    elapsed = time_us_32() - begin;
    speed = __ddu32_div(__ddu32_mul(d, a), elapsed);
    fgoutf(f, "32-bit line read speed: %f MBps\n", speed);
    return 0;
}
