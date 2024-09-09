extern "C" {
#include "m-os-api.h"
//#include "m-os-api-sdtfn.h"
//#include "m-os-api-timer.h"
//#include "m-os-api-math.h"
}

#define TOTAL_MEMORY (64 * 1024 + 256 * 1024)

#include "options.h"

int main(void) {
    cmd_ctx_t* ctx = get_cmd_ctx();
    marked_to_exit = false;

    Options.nosound = true;
    Options.nofilter = true;
    Options.log.fdc = false;


    return 0;
}

#include "options.cpp"
