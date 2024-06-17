#pragma once
#include <stdbool.h>

bool load_firmware(const char pathname[256]);
void run_app(char * name);

bool is_new_app(char * name);
int run_new_app(char * name, char * fn);
