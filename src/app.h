#pragma once
#include <stdbool.h>

bool load_firmware(const char pathname[256]);
void run_app(char * name);

bool is_new_app(char * name);
void run_new_app(char * name, char * fn);
