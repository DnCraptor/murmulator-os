#pragma once
#include <stdint.h>

void set_overclocking(uint32_t khz);
uint32_t get_overclocking_khz();
void overclocking();
void overclocking_ex(uint32_t vco, uint32_t postdiv1, uint32_t postdiv2);
