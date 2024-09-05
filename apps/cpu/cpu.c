#include "m-os-api.h"

int main() {
    cmd_ctx_t* ctx = get_cmd_ctx();
    if (ctx->argc == 1) {
        overclocking();
    } else if (ctx->argc == 2) {
        int cpu = atoi(ctx->argv[1]);
        if (!cpu) {
            goto usage;
        }
        if (cpu > 123 && cpu < 450) {
            set_overclocking(cpu * 1000);
            overclocking();
        } else {
            fgoutf(get_stderr(), "Unable to change CPU freq. to %s\n", ctx->argv[1]);
            goto usage;
        }
    } else if (ctx->argc == 4) {
        uint32_t vco = atoi(ctx->argv[1]);
        uint32_t div1 = atoi(ctx->argv[2]);
        uint32_t div2 = atoi(ctx->argv[3]);
        fgoutf(get_stdout(), "Attempt to set VCO: %dMHz; DIV1: %d; DIV2: %d\n", vco, div1, div2);
        set_sys_clock_pll(vco, div1, div2);
    } else {
usage:
        fgoutf(get_stderr(),
               "Unsupported options set. Use:\n"
               " - 'cpu' - show current settings;\n"
               " - 'cpu NNN' - set freq., where NNN - freq. in MHz;\n"
               " - 'cpu NNN1 N2 N3' - extended syntax,\n"
               "    where NNN1 - VCO freq. in MHz, N2 - div1, N3 - div2\n");
    }
    return 0;
}
