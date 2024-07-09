#include "m-os-api.h"

static void dma_handler_VGA_impl() {
}

int main(void) {
    cmd_ctx_t* ctx = get_cmd_ctx();
    set_dma_handler_impl(dma_handler_VGA_impl);
    return 0;
}

int __required_m_api_verion(void) {
    return M_API_VERSION;
}
