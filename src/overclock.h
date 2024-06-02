#pragma once
#include <stdint.h>

extern uint32_t overcloking_khz;

void overcloking();
void overcloking_ex(uint32_t vco, int32_t postdiv1, int32_t postdiv2);
