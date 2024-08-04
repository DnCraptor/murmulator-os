#include "m-os-api.h"

void* memset(void* p, int v, size_t sz) {
    typedef void* (*fn)(void *, int, size_t);
    return ((fn)_sys_table_ptrs[142])(p, v, sz);
}

int main(void) {
    int width = get_console_width();
    int height = get_console_height();
    int bytes = get_console_bitness() >> 3;
    uint8_t* b = get_buffer();
    printf("cls %dx%dx%d\n", width);
    memset(b, 0, width * height * bytes);
//    clrScr(0); - some issues
    graphics_set_con_pos(0, 1);
    graphics_set_con_color(7, 0);
    return 0;
}

int __required_m_api_verion(void) {
    return M_API_VERSION;
}
