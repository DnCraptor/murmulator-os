#include "m-os-api.h"
#include "font8x16.h"

void* memset(void* p, int v, size_t sz) {
    typedef void* (*fn)(void *, int, size_t);
    return ((fn)_sys_table_ptrs[142])(p, v, sz);
}

enum graphics_mode_t {
    TEXTMODE_80x30,
    TEXTMODE_128x48,
    BK_256x256x2,
    BK_512x256x1,
};

static volatile uint32_t frame_number;
static volatile uint32_t screen_line;
static volatile uint8_t* input_buffer;
static volatile uint32_t* * prev_output_buffer;

typedef struct {
    uint8_t* graphics_buffer;
    uint32_t* lines_pattern_data;
    // буфер 2К текстовой палитры для быстрой работы
    uint16_t* txt_palette_fast;
} vga_context_t;

static volatile vga_context_t* vga_context;

static uint32_t* lines_pattern[4];

static volatile int N_lines_total;
static volatile int N_lines_visible;
static volatile int line_VS_begin;
static volatile int line_VS_end;
static volatile int shift_picture;
static volatile int visible_line_size;

static volatile int graphics_buffer_shift_x;
static volatile int graphics_buffer_shift_y;
static volatile uint16_t graphics_buffer_height;
static volatile uint16_t graphics_pallette_idx;

static bool is_flash_line;
static bool is_flash_frame;

static uint16_t palette[16*4];

static uint32_t bg_color[2];
static uint16_t palette16_mask;
static uint32_t text_buffer_width;
static uint32_t text_buffer_height;
static uint8_t bitness;
static int line_size;

static uint16_t txt_palette[16];

static volatile int pos_x;
static volatile int pos_y;
static volatile bool cursor_blink_state;

static uint8_t con_color;
static uint8_t con_bgcolor;
static volatile bool lock_buffer;
static volatile int graphics_mode;

int _init(void) {
    frame_number = 0;
    screen_line = 0;
    input_buffer = NULL;
    prev_output_buffer = NULL;
    vga_context = NULL;
    N_lines_total = 525;
    N_lines_visible = 480;
    line_VS_begin = 490;
    line_VS_end = 491;
    shift_picture = 0;
    visible_line_size = 320;
    graphics_buffer_shift_x = 0;
    graphics_buffer_shift_y = 0;
    graphics_buffer_height = 256;
    graphics_pallette_idx = 0;
    is_flash_line = false;
    is_flash_frame = false;
    palette16_mask = 0;
    text_buffer_width = 0;
    text_buffer_height = 0;
    bitness = 16;
    line_size = 0;
    pos_x = 0;
    pos_y = 0;
    cursor_blink_state = true;
    con_color = 7;
    con_bgcolor = 0;
    lock_buffer = false;
    graphics_mode = -1;
}

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
            output_buffer_32bit += shift_picture >> 2;
            uint32_t p_i = ((screen_line & is_flash_line) + (frame_number & is_flash_frame)) & 1;
            uint32_t color32 = bg_color[p_i];
            for (int i = visible_line_size >> 1; i--;) {
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
    uint32_t div_factor = 2;
    switch (graphics_mode) {
        case BK_256x256x2:
        case BK_512x256x1:
            line_number = __u32u32u32_div(screen_line, 3);
            if (line_number * 3 != screen_line) { // три подряд строки рисуем одно и тоже
                if (prev_output_buffer) output_buffer = prev_output_buffer;
                return output_buffer;
            }
            prev_output_buffer = output_buffer;
            y = line_number - graphics_buffer_shift_y;
            break;
        case TEXTMODE_80x30: {
            uint16_t* output_buffer_16bit = (uint16_t *)*output_buffer;
            output_buffer_16bit += shift_picture >> 1;
            const uint32_t font_weight = 8;
            const uint32_t font_height = 16;
            // "слой" символа
            uint32_t glyph_line = screen_line % font_height;
            //указатель откуда начать считывать символы
            uint8_t* text_buffer_line = &input_buffer[__u32u32u32_div(screen_line * text_buffer_width * 2, font_height)];
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
            output_buffer_16bit += shift_picture >> 1;
            const uint32_t font_weight = 8;
            const uint32_t font_height = 16;
            uint16_t* txt_palette_fast = vga_context->txt_palette_fast;
            // "слой" символа
            register uint32_t glyph_line = screen_line % font_height;
            // указатель откуда начать считывать символы
            register uint8_t* text_buffer_line = &input_buffer[__u32u32u32_div(screen_line * text_buffer_width * 2, font_height)];
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
    uint32_t height = graphics_buffer_height;
    if (y >= height) {
        // заполнение линии цветом фона
        if ((y == height) | (y == (height + 1)) |
            (y == (height + 2))) {
            uint32_t* output_buffer_32bit = (uint32_t *)*output_buffer;
            uint32_t p_i = ((line_number & is_flash_line) + (frame_number & is_flash_frame)) & 1;
            uint32_t color32 = bg_color[p_i];
            output_buffer_32bit += shift_picture >> 2;
            for (int i = visible_line_size >> 1; i--;) {
                *output_buffer_32bit++ = color32;
            }
        }
        return output_buffer;
    };
    //зона прорисовки изображения
    int addr_in_buf = 64 * (y + graphics_buffer_shift_y - 0330);
    while (addr_in_buf < 0) addr_in_buf += 16 << 10;
    while (addr_in_buf >= 16 << 10) addr_in_buf -= 16 << 10;
    uint8_t* input_buffer_8bit = input_buffer + addr_in_buf;
    uint16_t* output_buffer_16bit = (uint16_t *)(*output_buffer);
    output_buffer_16bit += shift_picture >> 1; //смещение началы вывода на размер синхросигнала
    if (graphics_mode == BK_512x256x1) {
        graphics_buffer_shift_x &= 0xfffffff1; //1bit buf
    }
    else {
        graphics_buffer_shift_x &= 0xfffffff2; //2bit buf
    }
    //для div_factor 2
    uint32_t max_width = text_buffer_width;
    if (graphics_buffer_shift_x < 0) {
        if (BK_512x256x1 == graphics_mode) {
            input_buffer_8bit -= graphics_buffer_shift_x >> 3; //1bit buf
        }
        else {
            input_buffer_8bit -= graphics_buffer_shift_x >> 2; //2bit buf
        }
        max_width += graphics_buffer_shift_x;
    }
    else {
        output_buffer_16bit += __u32u32u32_div(graphics_buffer_shift_x * 2, div_factor);
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
            for (int x = 512 >> 3; x--;) {
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
            current_palette += graphics_pallette_idx * 4;
            int pallete_mask = 3; // 11 - 2 bits
            register uint16_t m = (3 << 6) | (pallete_mask << 4) | (pallete_mask << 2) | pallete_mask; // TODO: outside
            m |= m << 8;
            //2bit buf
            for (int x = 256 >> 2; x--;) {
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
