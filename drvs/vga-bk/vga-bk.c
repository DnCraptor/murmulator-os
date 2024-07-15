#include "m-os-api.h"
#include "font8x16.h"

void* memset(void* p, int v, size_t sz) {
    typedef void* (*fn)(void *, int, size_t);
    return ((fn)_sys_table_ptrs[142])(p, v, sz);
}

void* memcpy(void *__restrict dst, const void *__restrict src, size_t sz) {
    typedef void* (*fn)(void *, const void*, size_t);
    return ((fn)_sys_table_ptrs[167])(dst, src, sz);
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
    printf("0\n");
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
    return 0;
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

static void vga_cleanup(void) {
    vga_context_t* cleanup = vga_context;
    vga_context = 0;
    if (cleanup) {
        if (!lock_buffer && cleanup->graphics_buffer) vPortFree(cleanup->graphics_buffer);
        if (cleanup->lines_pattern_data) vPortFree(cleanup->lines_pattern_data);
        if (cleanup->txt_palette_fast) vPortFree(cleanup->txt_palette_fast);
        vPortFree(cleanup);
    }
}

bool vga_set_mode(int mode) {
    if (graphics_mode == mode) return true;
    vga_context_t* context = pvPortCalloc(1, sizeof(vga_context_t));
    switch (mode) {
        case TEXTMODE_80x30:
            text_buffer_width = 80;
            text_buffer_height = 30;
            bitness = 16;
            break;
        case TEXTMODE_128x48:
            text_buffer_width = 128;
            text_buffer_height = 48;
            bitness = 16;
            break;
        case BK_256x256x2:
            text_buffer_width = 256;
            text_buffer_height = 256;
            bitness = 2;
            break;
        case BK_512x256x1:
            text_buffer_width = 512;
            text_buffer_height = 256;
            bitness = 1;
            break;
        default:
            return false;
    }
    graphics_mode = mode;
    pos_x = 0;
    pos_y = 0;

    uint8_t TMPL_VHS8 = 0;
    uint8_t TMPL_VS8 = 0;
    uint8_t TMPL_HS8 = 0;
    uint8_t TMPL_LINE8 = 0;

    int HS_SIZE = 4;
    int HS_SHIFT = 100;

    switch (graphics_mode) {
        case TEXTMODE_80x30:
            TMPL_LINE8 = 0b11000000;
            HS_SHIFT = 328 * 2;
            HS_SIZE = 48 * 2;
            line_size = 400 * 2;
            shift_picture = line_size - HS_SHIFT;
            visible_line_size = 320;
            N_lines_total = 525;
            N_lines_visible = 480;
            line_VS_begin = 490;
            line_VS_end = 491;
            set_graphics_clkdiv(25175000, line_size); // частота пиксельклока
            break;
        case TEXTMODE_128x48:
        case BK_256x256x2:
        case BK_512x256x1:
            TMPL_LINE8 = 0b11000000;
            // XGA Signal 1024 x 768 @ 60 Hz timing
            HS_SHIFT = 1024 + 24; // Front porch + Visible area
            HS_SIZE = 160; // Back porch
            line_size = 1344;
            shift_picture = line_size - HS_SHIFT;
            visible_line_size = 512;
            N_lines_visible = 768;
            line_VS_begin = 768 + 3; // + Front porch
            line_VS_end = 768 + 3 + 6; // ++ Sync pulse 2?
            N_lines_total = 806; // Whole frame
            set_graphics_clkdiv(65000000, line_size); // частота пиксельклока 65.0 MHz
            break;
    }

    //инициализация шаблонов строк и синхросигнала
    if (mode == TEXTMODE_80x30) {
        context->lines_pattern_data = (uint32_t *)pvPortCalloc(line_size, sizeof(uint32_t));
        for (int i = 0; i < 4; i++) {
            lines_pattern[i] = &context->lines_pattern_data[i * (line_size >> 2)];
        }
        TMPL_VHS8 = TMPL_LINE8 ^ 0b11000000;
        TMPL_VS8 = TMPL_LINE8 ^ 0b10000000;
        TMPL_HS8 = TMPL_LINE8 ^ 0b01000000;
        uint8_t* base_ptr = (uint8_t *)lines_pattern[0];
        //пустая строка
        memset(base_ptr, TMPL_LINE8, line_size);
        //выровненная синхра вначале
        memset(base_ptr, TMPL_HS8, HS_SIZE);
        // кадровая синхра
        base_ptr = (uint8_t *)lines_pattern[1];
        memset(base_ptr, TMPL_VS8, line_size);
        //выровненная синхра вначале
        memset(base_ptr, TMPL_VHS8, HS_SIZE);
        //заготовки для строк с изображением
        base_ptr = (uint8_t *)lines_pattern[2];
        memcpy(base_ptr, lines_pattern[0], line_size);
        base_ptr = (uint8_t *)lines_pattern[3];
        memcpy(base_ptr, lines_pattern[0], line_size);
    } else {
        context->lines_pattern_data = (uint32_t *)pvPortCalloc(line_size, sizeof(uint32_t));;
        for (int i = 0; i < 4; i++) {
            lines_pattern[i] = &context->lines_pattern_data[i * (line_size >> 2)];
        }
        TMPL_VHS8 = TMPL_LINE8 ^ 0b11000000;
        TMPL_VS8 = TMPL_LINE8 ^ 0b10000000;
        TMPL_HS8 = TMPL_LINE8 ^ 0b01000000;
        uint8_t* base_ptr = (uint8_t *)lines_pattern[0];
        //пустая строка
        memset(base_ptr, TMPL_LINE8, line_size);
        //выровненная синхра вначале
        memset(base_ptr, TMPL_HS8, HS_SIZE);
        // кадровая синхра
        base_ptr = (uint8_t *)lines_pattern[1];
        memset(base_ptr, TMPL_VS8, line_size);
        //выровненная синхра вначале
        memset(base_ptr, TMPL_VHS8, HS_SIZE);
        //заготовки для строк с изображением
        base_ptr = (uint8_t *)lines_pattern[2];
        memcpy(base_ptr, lines_pattern[0], line_size);
        base_ptr = (uint8_t *)lines_pattern[3];
        memcpy(base_ptr, lines_pattern[0], line_size);
    }
    frame_number = 0;
    screen_line = 0;
    input_buffer = NULL;
    prev_output_buffer = 0;

    vga_context_t* cleanup = vga_context;
    switch (mode) {
        case TEXTMODE_80x30:
        case TEXTMODE_128x48:
            context->graphics_buffer = lock_buffer ? vga_context->graphics_buffer : pvPortCalloc(text_buffer_width * text_buffer_height, 2);
            context->txt_palette_fast = (uint16_t *)pvPortCalloc(256 * 4, sizeof(uint16_t));
            for (int i = 0; i < 256; i++) {
                uint8_t c1 = txt_palette[i & 0xf];
                uint8_t c0 = txt_palette[i >> 4];
                context->txt_palette_fast[i * 4 + 0] = (c0) | (c0 << 8);
                context->txt_palette_fast[i * 4 + 1] = (c1) | (c0 << 8);
                context->txt_palette_fast[i * 4 + 2] = (c0) | (c1 << 8);
                context->txt_palette_fast[i * 4 + 3] = (c1) | (c1 << 8);
            }
            break;
        case BK_256x256x2:
        case BK_512x256x1:
            context->graphics_buffer = lock_buffer ? vga_context->graphics_buffer : pvPortCalloc(512 >> 3, 256);
            break;
    }
    vga_context = context;
    if (cleanup) {
        if (cleanup->txt_palette_fast) vPortFree(cleanup->txt_palette_fast);
        if (cleanup->lines_pattern_data) vPortFree(cleanup->lines_pattern_data);
        if (!lock_buffer && cleanup->graphics_buffer) vPortFree(cleanup->graphics_buffer);
        vPortFree(cleanup);
    }
    vga_dma_channel_set_read_addr(&lines_pattern[0]);
    return true;
};

static bool vga_is_mode_text(int mode) {
    return mode <= TEXTMODE_128x48;
}

static bool vga_is_text_mode() {
    return vga_is_mode_text(graphics_mode);
}

static int vga_get_mode(void) {
    return graphics_mode;
}

static uint32_t get_vga_console_width() {
    return text_buffer_width;
}

static uint32_t get_vga_console_height() {
    return text_buffer_height;
}

static uint8_t* get_vga_buffer() {
    if (!vga_context) return NULL;
    return vga_context->graphics_buffer;
}

static void set_vga_buffer(uint8_t* buffer) {
    if (!vga_context) return;
    vga_context->graphics_buffer = buffer;
}

static size_t vga_buffer_size() {
    switch (graphics_mode) {
        case TEXTMODE_80x30:
        case TEXTMODE_128x48:
            return (lock_buffer ? 0 : text_buffer_height * text_buffer_width * 2)
                + 256 * 4 * sizeof(uint16_t) + line_size * sizeof(uint32_t);
        case BK_256x256x2:
        case BK_512x256x1:
        default:
            return (lock_buffer ? (512 >> 3) * 256 : 0) + line_size * sizeof(uint32_t);
    }
}

static void vga_set_con_pos(int x, int y) {
    pos_x = x;
    pos_y = y;
}

static void vga_set_con_color(uint8_t color, uint8_t bgcolor) {
    con_color = color;
    con_bgcolor = bgcolor;
}


static char* _rollup(char* t_buf) {
    char* b = vga_context->graphics_buffer;
    if (pos_y >= text_buffer_height - 1) {
        memcpy(b, b + text_buffer_width * 2, text_buffer_width * (text_buffer_height - 2) * 2);
        t_buf = b + text_buffer_width * (text_buffer_height - 2) * 2;
        for(int i = 0; i < text_buffer_width; ++i) {
            *t_buf++ = ' ';
            *t_buf++ = con_bgcolor << 4 | con_color & 0xF;
        }
        pos_y = text_buffer_height - 2;
    }
    return b + text_buffer_width * 2 * pos_y + 2 * pos_x;
}

static void vga_print(char* buf) {
    if (!vga_context || !vga_context->graphics_buffer) return;
    uint8_t* t_buf = vga_context->graphics_buffer + text_buffer_width * 2 * pos_y + 2 * pos_x;
    char c;
    while (c = *buf++) {
        if (c == '\r') continue; // ignore DOS stile \r\n, only \n to start new line
        if (c == '\n') {
            pos_x = 0;
            pos_y++;
            t_buf = _rollup(t_buf);
            continue;
        }
        pos_x++;
        if (pos_x >= text_buffer_width) {
            pos_x = 0;
            pos_y++;
            t_buf = _rollup(t_buf);
            *t_buf++ = c;
            *t_buf++ = con_bgcolor << 4 | con_color & 0xF;
            pos_x++;
        } else {
            *t_buf++ = c;
            *t_buf++ = con_bgcolor << 4 | con_color & 0xF;
        }
    }
}

static void vga_backspace(void) {
    if (!vga_context || !vga_context->graphics_buffer) return;
    uint8_t* t_buf;
    pos_x--;
    if (pos_x < 0) {
        pos_x = text_buffer_width - 2;
        pos_y--;
        if (pos_y < 0) {
            pos_y = 0;
        }
    }
    t_buf = vga_context->graphics_buffer + text_buffer_width * 2 * pos_y + 2 * pos_x;
    *t_buf++ = ' ';
    *t_buf++ = con_bgcolor << 4 | con_color & 0xF;
}

static void vga_lock_buffer(bool b) {
    lock_buffer = b;
}

static void vga_clr_scr(uint8_t color) {
    if (!vga_context || !vga_context->graphics_buffer) return;
    uint8_t* t_buf = vga_context->graphics_buffer;
    for (int yi = 0; yi < text_buffer_height; yi++)
        for (int xi = 0; xi < text_buffer_width * 2; xi++) {
            *t_buf++ = ' ';
            *t_buf++ = (color << 4) | (color & 0xF);
        }
    vga_set_con_pos(0, 0);
    vga_set_con_color(7, color); // TODO:
};

static void vga_draw_text(const char* string, int x, int y, uint8_t color, uint8_t bgcolor) {
    if (!vga_context || !vga_context->graphics_buffer) return;
    uint8_t* t_buf = vga_context->graphics_buffer + text_buffer_width * 2 * y + 2 * x;
    uint8_t c = (bgcolor << 4) | (color & 0xF);
    for (int xi = x; xi < text_buffer_width * 2; ++xi) {
        if (!(*string)) break;
        *t_buf++ = *string++;
        *t_buf++ = c;
    }
}

static uint8_t get_vga_buffer_bitness(void) {
    return bitness;
}

static void vga_set_bgcolor(const uint32_t color888) {
    const uint8_t conv0[] = { 0b00, 0b00, 0b01, 0b10, 0b10, 0b10, 0b11, 0b11 };
    const uint8_t conv1[] = { 0b00, 0b01, 0b01, 0b01, 0b10, 0b11, 0b11, 0b11 };

    const uint8_t b = __u32u32u32_div(color888 & 0xff, 42);
    const uint8_t r = __u32u32u32_div(color888 >> 16 & 0xff, 42);
    const uint8_t g = __u32u32u32_div(color888 >> 8 & 0xff, 42);

    const uint8_t c_hi = conv0[r] << 4 | conv0[g] << 2 | conv0[b];
    const uint8_t c_lo = conv1[r] << 4 | conv1[g] << 2 | conv1[b];
    bg_color[0] = ((c_hi << 8 | c_lo) & 0x3f3f | palette16_mask) << 16 |
                  ((c_hi << 8 | c_lo) & 0x3f3f | palette16_mask);
    bg_color[1] = ((c_lo << 8 | c_hi) & 0x3f3f | palette16_mask) << 16 |
                  ((c_lo << 8 | c_hi) & 0x3f3f | palette16_mask);
}

static int vga_con_x(void) {
    return pos_x;
}

static int vga_con_y(void) {
    return pos_y;
}


static void init_palette() {
    //инициализация палитры по умолчанию
    for (int i = 0; i < 16; ++i)
    { // black for 256*256*4
        palette[i*4] = 0xc0c0;
    }
    for (int i = 0; i < 16; ++i) {
        for (int j = 1; j < 4; ++j)
        { // white
            uint8_t rgb = 0b111111;
            uint8_t c = 0xc0 | rgb;
            palette[i*4+j] = (c << 8) | c;
        }
    }
    { // dark blue for 256*256*4
        uint8_t b = 0b11;
        uint8_t c = 0xc0 | b;
        palette[0*4+1] = (c << 8) | c;
        palette[2*4+2] = (c << 8) | c;
    }
    { // green for 256*256*4
        uint8_t g = 0b11 << 2;
        uint8_t c = 0xc0 | g;
        palette[0*4+2] = (c << 8) | c;
        palette[1*4+2] = (c << 8) | c;
        palette[3*4+1] = (c << 8) | c;
        palette[12*4+2] = (c << 8) | c;
        palette[14*4+2] = (c << 8) | c;
        palette[15*4+2] = (c << 8) | c;
    }
    { // red for 256*256*4
        uint8_t r = 0b11 << 4;
        uint8_t c = 0xc0 | r;
        palette[0*4+3] = (c << 8) | c;
        palette[1*4+3] = (c << 8) | c;
        palette[6*4+3] = (c << 8) | c;
        palette[11*4+3] = (c << 8) | c;
        palette[12*4+1] = (c << 8) | c;
    }
    { // yellow for 256*256*4
        uint8_t r = 0b11 << 4;
        uint8_t g = 0b11 << 2;
        uint8_t c = 0xc0 | r | g;
        palette[1*4+1] = (c << 8) | c;
        palette[3*4+3] = (c << 8) | c;
        palette[7*4+3] = (c << 8) | c;
        palette[11*4+2] = (c << 8) | c;
        palette[13*4+2] = (c << 8) | c;
        palette[14*4+1] = (c << 8) | c;
    }
    { // margenta for 256*256*4
        uint8_t r = 0b11 << 4;
        uint8_t b = 0b11;
        uint8_t c_hi = 0xc0 | r | b;
        palette[1*4+2] = (c_hi << 8) | c_hi;
        palette[2*4+3] = (c_hi << 8) | c_hi;
        palette[4*4+1] = (c_hi << 8) | c_hi;
        palette[8*4+3] = (c_hi << 8) | c_hi;
    }
    { // light blue for 256*256*4
        uint8_t r = 0b01 << 4;
        uint8_t g = 0b01 << 2;
        uint8_t b = 0b11;
        uint8_t c_hi = 0xc0 | r | g | b;
        palette[2*4+1] = (c_hi << 8) | c_hi;
        palette[3*4+2] = (c_hi << 8) | c_hi;
        palette[4*4+2] = (c_hi << 8) | c_hi;
        palette[11*4+1] = (c_hi << 8) | c_hi;
        palette[12*4+3] = (c_hi << 8) | c_hi;
        palette[13*4+1] = (c_hi << 8) | c_hi;
        palette[15*4+1] = (c_hi << 8) | c_hi;
    }
    { // deep red for 256*256*4
        uint8_t r = 0b10 << 4;
        uint8_t c_hi = 0xc0 | r;
        palette[6*4+1] = (c_hi << 8) | c_hi;
        palette[10*4+3] = (c_hi << 8) | c_hi;
    }
    { // dark red for 256*256*4
        uint8_t r = 0b01 << 4;
        uint8_t c_hi = 0xc0 | r;
        palette[6*4+2] = (c_hi << 8) | c_hi;
        palette[9*4+3] = (c_hi << 8) | c_hi;
    }
    { // yellow2 for 256*256*4
        uint8_t r = 0b10 << 4;
        uint8_t g = 0b10 << 2;
        uint8_t c_hi = 0xc0 | r | g;
        palette[7*4+1] = (c_hi << 8) | c_hi;
        palette[10*4+1] = (c_hi << 8) | c_hi;
    }
    { // yellow3 for 256*256*4
        uint8_t r = 0b10 << 4;
        uint8_t g = 0b11 << 2;
        uint8_t c_hi = 0xc0 | r | g;
        palette[7*4+2] = (c_hi << 8) | c_hi;
        palette[9*4+1] = (c_hi << 8) | c_hi;
    }
    { // margenta2 for 256*256*4
        uint8_t r = 0b10 << 4;
        uint8_t b = 0b10;
        uint8_t c_hi = 0xc0 | r | b;
        palette[8*4+1] = (c_hi << 8) | c_hi;
        palette[10*4+2] = (c_hi << 8) | c_hi;
    }
    { // margenta3 for 256*256*4
        uint8_t r = 0b10 << 4;
        uint8_t b = 0b10;
        uint8_t c_hi = 0xc0 | r | b;
        palette[8*4+2] = (c_hi << 8) | c_hi;
        palette[9*4+2] = (c_hi << 8) | c_hi;
    }
    //текстовая палитра
    for (int i = 0; i < 16; i++) {
        uint8_t b = (i & 1) ? ((i >> 3) ? 3 : 2) : 0;
        uint8_t r = (i & 4) ? ((i >> 3) ? 3 : 2) : 0;
        uint8_t g = (i & 2) ? ((i >> 3) ? 3 : 2) : 0;
        uint8_t c = (r << 4) | (g << 2) | b;
        txt_palette[i] = (c & 0x3f) | 0xc0;
    }
    palette16_mask = 0xc0c0;
    //корректировка  палитры по маске бит синхры
    bg_color[0] = (bg_color[0] & 0x3f3f3f3f) | palette16_mask | (palette16_mask << 16);
    bg_color[1] = (bg_color[1] & 0x3f3f3f3f) | palette16_mask | (palette16_mask << 16);
    for (int i = 0; i < 16*4; i++) {
        palette[i] = (palette[i] & 0x3f3f) | palette16_mask;
    }
}

static void vga_driver_init(void) {
    set_vga_dma_handler_impl(dma_handler_VGA_impl);
    vga_set_bgcolor(0x000000);
    init_palette();
}

int main(void) {
    printf("A\n");
    cmd_ctx_t* ctx = get_cmd_ctx();
    graphics_driver_t* gd0 = get_graphics_driver();
    graphics_driver_t* gd = malloc(sizeof(graphics_driver_t));
    gd->ctx = ctx;
    gd->init = vga_driver_init;
    gd->cleanup = vga_cleanup;
    gd->set_mode = vga_set_mode;
    gd->is_text = vga_is_text_mode;
    gd->console_width = get_vga_console_width;
    gd->console_height = get_vga_console_height;
    gd->screen_width = get_vga_console_width;
    gd->screen_height = get_vga_console_height;
    gd->buffer = get_vga_buffer;
    gd->set_buffer = set_vga_buffer;
    gd->cls = vga_clr_scr;
    gd->draw_text = vga_draw_text;
    gd->console_bitness = get_vga_buffer_bitness;
    gd->screen_bitness = get_vga_buffer_bitness;
    gd->set_offsets = 0;
    gd->set_bgcolor = vga_set_bgcolor;
    gd->allocated = vga_buffer_size;
    gd->set_con_pos = vga_set_con_pos;
    gd->pos_x = vga_con_x;
    gd->pos_y = vga_con_y;
    gd->set_con_color = vga_set_con_color;
    gd->print = vga_print;
    gd->backspace = vga_backspace;
    gd->lock_buffer = vga_lock_buffer;
    gd->get_mode = vga_get_mode;
    gd->is_mode_text = vga_is_text_mode;
    gd->set_clkdiv = gd0->set_clkdiv;
    printf("B\n");
    install_graphics_driver(gd);
    printf("D\n");
    vga_set_mode(0);
    printf("E\n");
    for(;;) {
        vTaskDelay(100);
        if (!vga_context) break;
    }
    return 0;
}

int __required_m_api_verion(void) {
    return M_API_VERSION;
}
