#pragma once
#include <stdbool.h>
#include "ff.h"
void cmd_up();
void cmd_down();
void cmd_enter();
void cmd_tab();
void cmd_backspace();
bool exists(char* t, const char * cmd);
void cmd_push(char c);
char* get_curr_dir();
FIL * get_stdout();
FIL * get_stderr();
