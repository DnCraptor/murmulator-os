#include "overclock.h"
#include "graphics.h"
#include "cmd.h"
#include <pico/stdlib.h>

static uint32_t overclocking_khz = OVERCLOCKING * 1000;

void set_overclocking(uint32_t khz) {
    overclocking_khz = khz;
}
uint32_t get_overclocking_khz() {
    return overclocking_khz;
}

void overclocking_ex(uint32_t vco, uint32_t postdiv1, uint32_t postdiv2) {
    overclocking_khz = vco / postdiv1 / postdiv2 / 1000;
    vTaskSuspendAll();;
    set_sys_clock_pll(vco, postdiv1, postdiv2);
	xTaskResumeAll();;
    FIL * f = get_stdout();
    fgoutf(f, "CPU: %f MHz (vco: %d MHz, pd1: %d, pd2: %d)\n", overclocking_khz / 1000.0, vco / 1000000, postdiv1, postdiv2);
}

void overclocking() {
    uint32_t vco, postdiv1, postdiv2;
    vTaskSuspendAll();;
    if (check_sys_clock_khz(overclocking_khz, &vco, &postdiv1, &postdiv2)) {
        overclocking_ex(vco, postdiv1, postdiv2);
    	xTaskResumeAll();;
    } else {
    	xTaskResumeAll();;
        FIL * f = get_stderr();
        fgoutf(f, "System clock of %d kHz cannot be achieved\n", overclocking_khz);
    }
}
