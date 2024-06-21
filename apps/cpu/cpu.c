#include "m-os-api.h"

int main() {
    cmd_startup_ctx_t* ctx = get_cmd_startup_ctx();
    if (ctx->tokens == 1) {
        overclocking();
    } else if (ctx->tokens == 2) {
        char* nt = next_token(ctx->cmd_t);
        int cpu = atoi(nt);
        if (!cpu) {
            goto usage;
        }
        if (cpu > 123 && cpu < 450) {
            set_overclocking(cpu * 1000);
            overclocking();
        } else {
            fgoutf(get_stderr(), "Unable to change CPU freq. to %s\n", nt);
            goto usage;
        }
    } else if (ctx->tokens == 4) {
        char* nt = next_token(ctx->cmd_t);
        uint32_t vco = atoi(nt);
        nt = next_token(nt);
        uint32_t div1 = atoi(nt);
        nt = next_token(nt);
        uint32_t div2 = atoi(nt);
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

int __required_m_api_verion() {
    return 2;
}
