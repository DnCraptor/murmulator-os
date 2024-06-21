#include "m-os-api.h"

int main() {
    clrScr(1);
    graphics_set_con_pos(0, 1);
    graphics_set_con_color(7, 0);
}

int __required_m_api_verion(void) {
    return 2;
}
