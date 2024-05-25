#pragma once

#include <stdint.h>
#include <stdbool.h>

uint32_t get_cpu_ram_size(void);
uint32_t get_cpu_flash_size(void);
bool cpu_check_address(volatile const char *address);
__attribute__((naked)) void hardfault_handler(void);
