#include "m-os-api.h"

int main(void) {
    cmd_ctx_t* ctx = get_cmd_ctx();
    if (ctx->argc != 2) {
        fprintf(ctx->std_err, "Usage: gmode #\n where # - integer number\n");
        return 1;
    }
    int mode = atoi(ctx->argv[1]);
    if (graphics_is_mode_text(mode)) {
        fprintf(ctx->std_err, "mode #%d is marked for text output\n", mode);
        return -2;
    }
    int prev = graphics_get_mode();
    if (!graphics_set_mode(mode)) {
        fprintf(ctx->std_err, "Unsupported mode #%d\n", mode);
        return -1;
    }
    uint32_t w = get_screen_width();
    uint32_t h = get_screen_height();
    uint8_t bit = get_screen_bitness();
    uint8_t* buff = get_buffer();
    if (bit < 4) {
        uint32_t wb = bit == 1 ? (w >> 3) : (bit == 2 ? (w >> 2) : (w >> 1));
        if (bit == 1) {
            for(uint32_t x = 0; x < wb; ++x) {
                for(uint32_t y = 0; y < h; ++y) {
                    *(buff + y * wb + x) = 0xFF;
                }
            }
        } else {
            for(uint32_t x = 0; x < wb; ++x) {
                for(uint32_t y = 0; y < h; ++y) {
                    *(buff + y * wb + x) = 0b11111111;
                }
            }
            vTaskDelay(5000);
            for(uint32_t x = 0; x < wb; ++x) {
                for(uint32_t y = 0; y < h; ++y) {
                    *(buff + y * wb + x) = 0b10101010;
                }
            }
            vTaskDelay(1000);
            for(uint32_t x = 0; x < wb; ++x) {
                for(uint32_t y = 0; y < h; ++y) {
                    *(buff + y * wb + x) = 0b01010101;
                }
            }
        }
    } else {
        vTaskDelay(5000);
        /*
        size_t sz = w * h * (bit >> 3);
        for (uint8_t c = 0; c < 256; ++c) {
            __memset(buff, c, sz);
            vTaskDelay(100);
        }
        */
        size_t sz = (w * h * bit) >> 3;
        for (size_t off = 0; off < sz; ++off) {
            buff[off] = off & 0xFF;
        }
    }
    getch();
    graphics_set_mode(prev);
    graphics_set_con_pos(0, 0);
    printf("Mode info: %d x %d x %d bits\n", w, h, bit);
    return 0;
}

int __required_m_api_verion(void) {
    return M_API_VERSION;
}
