#include "m-os-api.h"

inline static int usb_on() {
    init_pico_usb_drive();

    while (!tud_msc_ejected()) {
        pico_usb_drive_heartbeat();
    }
    printf("USB-drive was ejected...\n");
    int post_cicles = 100;
    while (--post_cicles) {
        pico_usb_drive_heartbeat();
    }
    return 0;
}

inline static int usb_off() {
    set_tud_msc_ejected(true);
    return 0;
}

int main(void) {
    cmd_ctx_t* ctx = get_cmd_ctx();
    if (ctx->argc == 0) {
        fprintf(ctx->std_err, "ATTTENTION! BROKEN EXECUTION CONTEXT [%p]!\n", ctx);
        return -1;
    }
    if (ctx->argc == 1) {
        return usb_on();
    }
    if (ctx->argc == 2) { // usb [on/off]
        if (strncmp(ctx->argv[1], "on", 2) == 0) return usb_on();
        if (strncmp(ctx->argv[1], "off", 3) == 0) return usb_off();
    }
    if (ctx->argc > 2) {
        fprintf(ctx->std_err,
                "Unexpected number of argument: %d\n"
                "Use: usb [on/off] &\n", ctx->argc
        );
        return 3;
    }
    fprintf(ctx->std_err,
            "Unexpected argument: %s\n"
            "Use: usb [on/off] &\n", ctx->argv[1]
    );
    return 4;
}

int __required_m_api_verion(void) {
    return M_API_VERSION;
}
