#include "m-os-api.h"

int main(void) {
    clrScr(1);
    graphics_set_con_pos(0, 1);
    graphics_set_con_color(7, 0);
    return 0;
}

int __required_m_api_verion(void) {
    return M_API_VERSION;
}
