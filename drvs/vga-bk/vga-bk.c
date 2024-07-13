#include "m-os-api.h"

static volatile uint32_t frame_number = 0;
static volatile uint32_t screen_line = 0;
static volatile uint8_t* input_buffer = NULL;
static volatile uint32_t* * prev_output_buffer = 0;

static uint8_t* dma_handler_VGA_impl() {
    screen_line++;

    if (screen_line == N_lines_total) {
        screen_line = 0;
        frame_number++;
        input_buffer = vga_context ? vga_context->graphics_buffer : NULL;
    }

    if (screen_line >= N_lines_visible) {
        //заполнение цветом фона
        if ((screen_line == N_lines_visible) | (screen_line == (N_lines_visible + 3))) {
            uint32_t* output_buffer_32bit = lines_pattern[2 + ((screen_line) & 1)];
            output_buffer_32bit += shift_picture / 4;
            uint32_t p_i = ((screen_line & is_flash_line) + (frame_number & is_flash_frame)) & 1;
            uint32_t color32 = bg_color[p_i];
            for (int i = visible_line_size / 2; i--;) {
                *output_buffer_32bit++ = color32;
            }
        }

        // синхросигналы
        if ((screen_line >= line_VS_begin) && (screen_line <= line_VS_end))
            return &lines_pattern[1]; // VS SYNC
        return &lines_pattern[0];
    }

    if (!(input_buffer)) {
        return &lines_pattern[0];
    } //если нет видеобуфера - рисуем пустую строку

    int y, line_number;

    uint32_t* * output_buffer = &lines_pattern[2 + (screen_line & 1)];
    uint div_factor = 2;
    switch (graphics_mode) {
        case BK_256x256x2:
        case BK_512x256x1:
            if (screen_line % 3 != 0) { // три подряд строки рисуем одно и тоже
                if (prev_output_buffer) output_buffer = prev_output_buffer;
                return output_buffer;
            }
            prev_output_buffer = output_buffer;
            line_number = screen_line / 3;
            y = line_number - graphics_buffer_shift_y;
            break;
        case TEXTMODE_80x30: {
            uint16_t* output_buffer_16bit = (uint16_t *)*output_buffer;
            output_buffer_16bit += shift_picture / 2;
            const uint font_weight = 8;
            const uint font_height = 16;
            // "слой" символа
            uint32_t glyph_line = screen_line % font_height;
            //указатель откуда начать считывать символы
            uint8_t* text_buffer_line = &input_buffer[screen_line / font_height * text_buffer_width * 2];
            uint16_t* txt_palette_fast = vga_context->txt_palette_fast;
            for (int x = 0; x < text_buffer_width; x++) {
                //из таблицы символов получаем "срез" текущего символа
                uint8_t glyph_pixels = font_8x16[(*text_buffer_line++) * font_height + glyph_line];
                //считываем из быстрой палитры начало таблицы быстрого преобразования 2-битных комбинаций цветов пикселей
                uint16_t* color = &txt_palette_fast[4 * (*text_buffer_line++)];
                if (cursor_blink_state && (screen_line >> 4) == pos_y && x == pos_x && glyph_line >= 13) { // TODO: cur height
                    color = &txt_palette_fast[0];
                    uint16_t c = color[7]; // TODO: setup cursor color
                    *output_buffer_16bit++ = c;
                    *output_buffer_16bit++ = c;
                    *output_buffer_16bit++ = c;
                    *output_buffer_16bit++ = c;
                } else {
                    *output_buffer_16bit++ = color[glyph_pixels & 3];
                    glyph_pixels >>= 2;
                    *output_buffer_16bit++ = color[glyph_pixels & 3];
                    glyph_pixels >>= 2;
                    *output_buffer_16bit++ = color[glyph_pixels & 3];
                    glyph_pixels >>= 2;
                    *output_buffer_16bit++ = color[glyph_pixels & 3];
                }
            }
            return output_buffer;
        }
        case TEXTMODE_128x48: {
            register uint16_t* output_buffer_16bit = (uint16_t *)*output_buffer;
            output_buffer_16bit += shift_picture / 2;
            const uint font_weight = 8;
            const uint font_height = 16;
            uint16_t* txt_palette_fast = vga_context->txt_palette_fast;
            // "слой" символа
            register uint32_t glyph_line = screen_line % font_height;
            //указатель откуда начать считывать символы
            register uint8_t* text_buffer_line = &input_buffer[screen_line / font_height * text_buffer_width * 2];
            register int xc = pos_x;
            if (cursor_blink_state && (screen_line >> 4) == pos_y && glyph_line >= 13) {
                register uint16_t* color = &txt_palette_fast[0];
                uint16_t c = color[7]; // TODO: setup cursor color
                for (register uint32_t x = 0; x < text_buffer_width; x++) {
                    //из таблицы символов получаем "срез" текущего символа
                    uint8_t glyph_pixels = font_8x16[(*text_buffer_line++) * font_height + glyph_line];
                    //считываем из быстрой палитры начало таблицы быстрого преобразования 2-битных комбинаций цветов пикселей
                    color = &txt_palette_fast[4 * (*text_buffer_line++)];
                    if (x == xc) { // TODO: cur height
                        *output_buffer_16bit++ = c;
                        *output_buffer_16bit++ = c;
                        *output_buffer_16bit++ = c;
                        *output_buffer_16bit++ = c;
                    } else {
                        *output_buffer_16bit++ = color[glyph_pixels & 3];
                        glyph_pixels >>= 2;
                        *output_buffer_16bit++ = color[glyph_pixels & 3];
                        glyph_pixels >>= 2;
                        *output_buffer_16bit++ = color[glyph_pixels & 3];
                        glyph_pixels >>= 2;
                        *output_buffer_16bit++ = color[glyph_pixels & 3];
                    }
                }
            } else {
              for (register uint32_t x = 0; x < text_buffer_width; x++) {
                    //из таблицы символов получаем "срез" текущего символа
                    uint8_t glyph_pixels = font_8x16[(*text_buffer_line++) * font_height + glyph_line];
                    //считываем из быстрой палитры начало таблицы быстрого преобразования 2-битных комбинаций цветов пикселей
                    uint16_t* color = &txt_palette_fast[4 * (*text_buffer_line++)];
                    *output_buffer_16bit++ = color[glyph_pixels & 3];
                    glyph_pixels >>= 2;
                    *output_buffer_16bit++ = color[glyph_pixels & 3];
                    glyph_pixels >>= 2;
                    *output_buffer_16bit++ = color[glyph_pixels & 3];
                    glyph_pixels >>= 2;
                    *output_buffer_16bit++ = color[glyph_pixels & 3];
                }
            }
            return output_buffer;
        }
        default: {
            return &lines_pattern[0];
        }
    }
    if (y < 0) {
        return &lines_pattern[0];
    }
    uint graphics_buffer_height = g_conf.graphics_buffer_height;
    if (y >= graphics_buffer_height) {
        // заполнение линии цветом фона
        if ((y == graphics_buffer_height) | (y == (graphics_buffer_height + 1)) |
            (y == (graphics_buffer_height + 2))) {
            uint32_t* output_buffer_32bit = (uint32_t *)*output_buffer;
            uint32_t p_i = ((line_number & is_flash_line) + (frame_number & is_flash_frame)) & 1;
            uint32_t color32 = bg_color[p_i];
            output_buffer_32bit += shift_picture / 4;
            for (int i = visible_line_size / 2; i--;) {
                *output_buffer_32bit++ = color32;
            }
        }
        return output_buffer;
    };
    //зона прорисовки изображения
    int addr_in_buf = 64 * (y + g_conf.shift_y - 0330);
    while (addr_in_buf < 0) addr_in_buf += 16 << 10;
    while (addr_in_buf >= 16 << 10) addr_in_buf -= 16 << 10;
    uint8_t* input_buffer_8bit = input_buffer + addr_in_buf;
    uint16_t* output_buffer_16bit = (uint16_t *)(*output_buffer);
    output_buffer_16bit += shift_picture / 2; //смещение началы вывода на размер синхросигнала
    if (graphics_mode == BK_512x256x1) {
        graphics_buffer_shift_x &= 0xfffffff1; //1bit buf
    }
    else {
        graphics_buffer_shift_x &= 0xfffffff2; //2bit buf
    }
    //для div_factor 2
    uint max_width = text_buffer_width;
    if (graphics_buffer_shift_x < 0) {
        if (BK_512x256x1 == graphics_mode) {
            input_buffer_8bit -= graphics_buffer_shift_x / 8; //1bit buf
        }
        else {
            input_buffer_8bit -= graphics_buffer_shift_x / 4; //2bit buf
        }
        max_width += graphics_buffer_shift_x;
    }
    else {
        output_buffer_16bit += graphics_buffer_shift_x * 2 / div_factor;
    }
    int width = MIN((visible_line_size - ((graphics_buffer_shift_x > 0) ? (graphics_buffer_shift_x) : 0)), max_width);
    if (width < 0) return; // TODO: detect a case
    // TODO: Упростить
    uint16_t* current_palette = &palette[0];
    uint8_t* output_buffer_8bit = (uint8_t *)output_buffer_16bit;
    switch (graphics_mode) {
        case BK_512x256x1: {
            current_palette += 5*4;
            //1bit buf
            for (int x = 512/8; x--;) {
                register uint8_t i = *input_buffer_8bit++;
                for (register uint8_t shift = 0; shift < 8; ++shift) {
                    register uint8_t t = current_palette[(i >> shift) & 1];
                    *output_buffer_8bit++ = t;
                    *output_buffer_8bit++ = t;
                }
            }
            break;
        }
        case BK_256x256x2: {
            current_palette += g_conf.graphics_pallette_idx * 4;
            register uint16_t m = (3 << 6) | (pallete_mask << 4) | (pallete_mask << 2) | pallete_mask; // TODO: outside
            m |= m << 8;
            //2bit buf
            for (int x = 256 / 4; x--;) {
                register uint8_t i = *input_buffer_8bit++;
                for (register uint8_t shift = 0; shift < 8; shift += 2) {
                    register uint8_t t = current_palette[(i >> shift) & 3] & m;
                    *output_buffer_8bit++ = t;
                    *output_buffer_8bit++ = t;
                    *output_buffer_8bit++ = t;
                    *output_buffer_8bit++ = t;
                }
            }
            break;
        }
        default:
            break;
    }
    return output_buffer;
}

int main(void) {
    cmd_ctx_t* ctx = get_cmd_ctx();
    graphics_driver_t* gd = malloc(sizeof(graphics_driver_t));
    gd->ctx = ctx;
    gd->init = 0;
    gd->cleanup = 0;
    gd->set_mode = 0;
    gd->is_text = 0;
    gd->console_width = 0;
    gd->console_height = 0;
    gd->screen_width = 0;
    gd->screen_height = 0;
    gd->buffer = 0;
    gd->set_buffer = 0;
    gd->cls = 0;
    gd->draw_text = 0;
    gd->console_bitness = 0;
    gd->screen_bitness = 0;
    gd->set_offsets = 0;
    gd->set_bgcolor = 0;
    gd->allocated = 0;
    gd->set_con_pos = 0;
    gd->pos_x = 0;
    gd->pos_y = 0;
    gd->set_con_color = 0;
    gd->print = 0;
    gd->backspace = 0;
    gd->lock_buffer = 0;
    gd->get_mode = 0;
    gd->is_mode_text = 0;
    install_graphics_driver(gd);
    set_dma_handler_impl(dma_handler_VGA_impl);
    return 0;
}

int __required_m_api_verion(void) {
    return M_API_VERSION;
}
