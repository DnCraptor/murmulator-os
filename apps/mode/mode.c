#include "m-os-api.h"

int main(void) {
    cmd_ctx_t* ctx = get_cmd_ctx();
    graphics_driver_t* gd = get_graphics_driver();
    install_graphics_driver(gd);
    return 0;
}

int __required_m_api_verion(void) {
    return M_API_VERSION;
}
