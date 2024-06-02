#include "overclock.h"
#include "graphics.h"
#include <pico/stdlib.h>

uint32_t overcloking_khz = OVERCLOCKING * 1000;

void overcloking_ex(uint32_t vco, int32_t postdiv1, int32_t postdiv2) {
    overcloking_khz = vco / postdiv1 / postdiv2 / 1000;
    set_sys_clock_pll(vco, postdiv1, postdiv2);
    goutf("CPU: %f MHz (vco: %d MHz, pd1: %d, pd2: %d)\n", overcloking_khz / 1000.0, vco / 1000000, postdiv1, postdiv2);
}

void overcloking() {
    uint32_t vco, postdiv1, postdiv2;
    if (check_sys_clock_khz(overcloking_khz, &vco, &postdiv1, &postdiv2)) {
        overcloking_ex(vco, postdiv1, postdiv2);
    } else {
        goutf("System clock of %d kHz cannot be achieved\n", overcloking_khz);
    }
}

