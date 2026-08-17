#include "m-os-api.h"
#include "m-os-api-timer.h"

int main() {
    marked_to_exit = false;
    FIL * f = get_stdout();
    uint32_t sz = swap_size();
    fgoutf(f, "SWAP size: %d bytes, base RAM at %ph (%d KB)\nPages index at %ph (for %d RAM pages, %d KB each one)\n",
              sz, swap_base(), swap_base_size() >> 10, swap_pages_base(), swap_pages(), swap_page_size() >> 10);
    if (!sz) return 0;
    uint32_t a = 0;
    uint32_t begin = time_us_32();
    for (; a < sz && !marked_to_exit; ++a) {
        ram_page_write(a, a & 0xFF);
    }
    uint32_t elapsed = time_us_32() - begin;
    double d = 1.0;
    double speed = __ddu32_div(__ddu32_mul(d, a), elapsed);
    fgoutf(f, "8-bit line write speed: %f MBps\n", speed);
    if (marked_to_exit) return 0;

    begin = time_us_32();
    for (a = 0; a < sz && !marked_to_exit; ++a) {
        uint8_t b = ram_page_read(a);
        if ((a & 0xFF) != b) {
            fgoutf(f, "8-bit read failed at %ph (%02Xh)\n", a, b);
            break;
        }
    }
    elapsed = time_us_32() - begin;
    speed = __ddu32_div(__ddu32_mul(d, a), elapsed);
    fgoutf(f, "8-bit line read speed: %f MBps\n", speed);
    if (marked_to_exit) return 0;

    begin = time_us_32();
    for (a = 0; a < sz && !marked_to_exit; a += 2) {
        ram_page_write16(a, a & 0xFFFF);
    }
    elapsed = time_us_32() - begin;
    speed = __ddu32_div(__ddu32_mul(d, a), elapsed);
    fgoutf(f, "16-bit line write speed: %f MBps\n", speed);
    if (marked_to_exit) return 0;

    begin = time_us_32();
    for (a = 0; a < sz && !marked_to_exit; a += 2) {
        uint16_t b = ram_page_read16(a);
        if ((a & 0xFFFF) != b) {
            fgoutf(f, "16-bit read failed at %ph (%04Xh)\n", a, b);
            break;
        }
    }
    elapsed = time_us_32() - begin;
    speed = __ddu32_div(__ddu32_mul(d, a), elapsed);
    fgoutf(f, "16-bit line read speed: %f MBps\n", speed);
    if (marked_to_exit) return 0;

    begin = time_us_32();
    for (a = 0; a < sz && !marked_to_exit; a += 4) {
        ram_page_write32(a, a);
    }
    elapsed = time_us_32() - begin;
    speed = __ddu32_div(__ddu32_mul(d, a), elapsed);
    fgoutf(f, "32-bit line write speed: %f MBps\n", speed);
    if (marked_to_exit) return 0;

    begin = time_us_32();
    for (a = 0; a < sz && !marked_to_exit; a += 4) {
        uint32_t b = ram_page_read32(a);
        if (a != b) {
            fgoutf(f, "32-bit read failed at %ph (%ph)\n", a, b);
            break;
        }
    }
    elapsed = time_us_32() - begin;
    speed = __ddu32_div(__ddu32_mul(d, a), elapsed);
    fgoutf(f, "32-bit line read speed: %f MBps\n", speed);
    return 0;
}
