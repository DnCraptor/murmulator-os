#include "vga.h"
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "fnt8x16.h"
#include "ram_page.h"
#include "FreeRTOS.h"
#include "task.h"

int pallete_mask = 3; // 11 - 2 bits

enum graphics_mode_t {
    TEXTMODE_53x30, // 640*480 (320*200) tba
    TEXTMODE_80x30, // 640*480
    TEXTMODE_100x37, // 800*600
    TEXTMODE_128x48, // 1024*768
    BK_256x256x2, // 1024*768 3-lines->1 4-pixels->1
    BK_512x256x1, // 1024*768 3-lines->1 2-pixels->1
//    GRAPHICS_320x240x1, // 640*480 with duplicated points/lines 8k for 1-bit
//    GRAPHICS_320x240x16, // 640*480 with duplicated points/lines 32k for 4 bit
    GRAPHICS_320x240x256, // 640*480 with duplicated points/lines 75k for 256 colors
//    GRAPHICS_640x480x1, // 640*480 38.4k for 1-bit
//    GRAPHICS_640x480x4, // 640*480 76.8k for 2-bit
//    GRAPHICS_640x480x16, // 640*480 153.6k for 4-bit
//    GRAPHICS_800x600x1, // 800*600 60k for 1-bit
};

int vga_get_default_mode(void) {
    return TEXTMODE_100x37;
}

typedef struct {
    uint8_t* graphics_buffer;
    uint32_t* lines_pattern_data;
    // буфер 2К текстовой палитры для быстрой работы
    uint16_t* txt_palette_fast;
} vga_context_t;

static volatile vga_context_t* vga_context = NULL;

static uint32_t* lines_pattern[4];
static volatile int N_lines_total = 525;
static volatile int N_lines_visible = 480;
static volatile int line_VS_begin = 490;
static volatile int line_VS_end = 491;
static volatile int shift_picture = 0;
static volatile int visible_line_size = 320;

static volatile int graphics_buffer_shift_x = 0;
static volatile int graphics_buffer_shift_y = 0;

static bool is_flash_line = false;
static bool is_flash_frame = false;

//буфер 1к графической палитры
static uint16_t __scratch_y("vga_driver") palette[16*4];

static uint32_t bg_color[2];
static uint16_t palette16_mask = 0;
static uint8_t* text_buf_color;
static uint text_buffer_width = 0;
static uint text_buffer_height = 0;
static uint8_t bitness = 16;
static int line_size = 0;

static uint16_t __scratch_y("vga_driver") txt_palette[16];

extern volatile int pos_x;
extern volatile int pos_y;
volatile bool __scratch_y("vga_driver_text") cursor_blink_state = true;

extern volatile uint8_t con_color;
extern volatile uint8_t con_bgcolor;

volatile uint8_t __scratch_y("driver_text") _cursor_color = 7;
volatile bool lock_buffer = false;
static volatile int graphics_mode = -1;

// TODO: separate header for sound mixer

// регистр "защёлка" для примитивного ковокса без буфера
volatile uint16_t true_covox = 0;
volatile uint16_t az_covox_L = 0;
volatile uint16_t az_covox_R = 0;
volatile uint16_t covox_mix = 0x0F;

typedef struct config_em {
    uint8_t graphics_pallette_idx;
    uint8_t shift_y;
} config_em_t;

volatile config_em_t g_conf = { 0 };

void vga_lock_buffer(bool b) {
    lock_buffer = b;
}
void vga_set_con_color(uint8_t color, uint8_t bgcolor) {
    con_color = color;
    con_bgcolor = bgcolor;
}
void vga_set_cursor_color(uint8_t color) {
    _cursor_color = color;
}
uint32_t get_vga_console_width() {
    return text_buffer_width;
}
uint32_t get_vga_console_height() {
    return text_buffer_height;
}
uint8_t* get_vga_buffer() {
    if (!vga_context) return NULL;
    return vga_context->graphics_buffer;
}
void set_vga_buffer(uint8_t* buffer) {
    if (!vga_context) return;
    vga_context->graphics_buffer = buffer;
}
int vga_con_x(void) {
    return pos_x;
}
int vga_con_y(void) {
    return pos_y;
}
void vga_set_con_pos(int x, int y) {
    pos_x = x;
    pos_y = y;
}
size_t vga_buffer_size() {
    switch (graphics_mode) {
        case TEXTMODE_53x30:
        case TEXTMODE_80x30:
        case TEXTMODE_100x37:
        case TEXTMODE_128x48:
            return (lock_buffer ? 0 : text_buffer_height * text_buffer_width * 2)
                + 256 * 4 * sizeof(uint16_t) + line_size * sizeof(uint32_t);
        case GRAPHICS_320x240x256:
            return (lock_buffer ? 320 * 240 : 0) + line_size * sizeof(uint32_t);
        case BK_256x256x2:
        case BK_512x256x1:
        default:
            return (lock_buffer ? 512 / 8 * 256 : 0) + line_size * sizeof(uint32_t);
    }
}
uint8_t get_vga_buffer_bitness(void) {
    return bitness;
}
void vga_cleanup(void) {
    vga_context_t* cleanup = vga_context;
    vga_context = 0;
    if (cleanup) {
        if (!lock_buffer && cleanup->graphics_buffer) vPortFree(cleanup->graphics_buffer);
        if (cleanup->lines_pattern_data) vPortFree(cleanup->lines_pattern_data);
        if (cleanup->txt_palette_fast) vPortFree(cleanup->txt_palette_fast);
        vPortFree(cleanup);
    }
}

void graphics_inc_palleter_offset() {
    g_conf.graphics_pallette_idx++;
    if (g_conf.graphics_pallette_idx > 0b1111) g_conf.graphics_pallette_idx = 0;
}

static volatile uint32_t frame_number = 0;
static volatile uint32_t screen_line = 0;
static volatile uint8_t* input_buffer = NULL;
static volatile uint32_t* * prev_output_buffer = 0;

static uint8_t* __time_critical_func(dma_handler_VGA_impl)() {
    screen_line++;

    if (screen_line == N_lines_total) {
        screen_line = 0;
        frame_number++;
        input_buffer = vga_context ? vga_context->graphics_buffer : NULL;
    }

    if (screen_line >= N_lines_visible) {
        // заполнение цветом фона
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

    if (!(input_buffer)) {  // если нет видеобуфера - рисуем пустую строку
        return &lines_pattern[0];
    }

    int y, line_number;

    uint32_t* * output_buffer = &lines_pattern[2 + (screen_line & 1)];
    uint div_factor = 2;
    int cursor_color = _cursor_color;
    switch (graphics_mode) {
        case BK_256x256x2:
        case BK_512x256x1:
            line_number = screen_line / 3;
            if (screen_line != line_number * 3) { // три подряд строки рисуем одно и тоже
                if (prev_output_buffer) output_buffer = prev_output_buffer;
                return output_buffer;
            }
            prev_output_buffer = output_buffer;
            y = line_number - graphics_buffer_shift_y;
            break;
        case GRAPHICS_320x240x256: 
            line_number = screen_line >> 1;
            if (screen_line != line_number << 1) { // duplicate lines
                if (prev_output_buffer) output_buffer = prev_output_buffer;
                return output_buffer;
            }
            prev_output_buffer = output_buffer;
            y = line_number - graphics_buffer_shift_y;
            break;
        case TEXTMODE_53x30:
        case TEXTMODE_80x30:
        case TEXTMODE_100x37:
        case TEXTMODE_128x48: {
            register uint16_t* output_buffer_16bit = (uint16_t *)*output_buffer;
            output_buffer_16bit += shift_picture / 2;
            uint16_t* txt_palette_fast = vga_context->txt_palette_fast;
            // "слой" символа
            register uint32_t glyph_line = screen_line & 0xF; // % font_height;
            // указатель откуда начать считывать символы
            register uint8_t* text_buffer_line = &input_buffer[(screen_line >> 4) * text_buffer_width * 2];
            register int xc = pos_x;
            if (cursor_blink_state && (screen_line >> 4) == pos_y && glyph_line >= 13) {
                register uint16_t* color = &txt_palette_fast[0];
                uint16_t c = color[cursor_color];
                for (register uint32_t x = 0; x < text_buffer_width; x++) {
                    // из таблицы символов получаем "срез" текущего символа
                    uint8_t glyph_pixels = font_8x16[((*text_buffer_line++) << 4) + glyph_line];
                    // считываем из быстрой палитры начало таблицы быстрого преобразования 2-битных комбинаций цветов пикселей
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
                    // из таблицы символов получаем "срез" текущего символа
                    uint8_t glyph_pixels = font_8x16[((*text_buffer_line++) << 4) + glyph_line];
                    // считываем из быстрой палитры начало таблицы быстрого преобразования 2-битных комбинаций цветов пикселей
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
    uint graphics_buffer_height = text_buffer_height;
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
    switch(graphics_mode) {
        case BK_512x256x1:
            graphics_buffer_shift_x &= 0xfffffff1; // 1-bit buf
            break;
        case BK_256x256x2:
            graphics_buffer_shift_x &= 0xfffffff2; // 2-bit buf
            break;
        case GRAPHICS_320x240x256:
            graphics_buffer_shift_x &= 0xfffffff0; // 8-bit buf
            break;
    }
    //для div_factor 2
    uint max_width = text_buffer_width;
    if (graphics_buffer_shift_x < 0) {
        switch(graphics_mode) {
            case BK_512x256x1:
                input_buffer_8bit -= graphics_buffer_shift_x >> 3; // 1-bit buf
                break;
            case BK_256x256x2:
                input_buffer_8bit -= graphics_buffer_shift_x >> 2; // 2-bit buf
                break;
            case GRAPHICS_320x240x256:
                input_buffer_8bit -= graphics_buffer_shift_x; // 8-bit buf
                break;
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
        case GRAPHICS_320x240x256: {
            for (register int x = 320; x--;) {
                register uint8_t t = 0xc0 | *input_buffer_8bit++;
                *output_buffer_8bit++ = t;
                *output_buffer_8bit++ = t;
            }
            break;
        }
        default:
            break;
    }
    return output_buffer;
}

#define MAX_WIDTH 128
#define MAX_HEIGHT 48
#define BYTES_PER_CHAR 2

bool vga_is_mode_text(int mode) {
    return mode <= TEXTMODE_128x48;
}

bool vga_is_text_mode() {
    return vga_is_mode_text(graphics_mode);
}

int vga_get_mode(void) {
    return graphics_mode;
}

bool vga_set_mode(int mode) {
  //  if (_SM_VGA < 0) return false; // если  VGA не инициализирована -
    if (graphics_mode == mode) return true;
    vga_context_t* context = pvPortCalloc(1, sizeof(vga_context_t));
    switch (mode) {
        case TEXTMODE_53x30:
            text_buffer_width = 53;
            text_buffer_height = 30;
            bitness = 16;
            break;
        case TEXTMODE_80x30:
            text_buffer_width = 80;
            text_buffer_height = 30;
            bitness = 16;
            break;
        case TEXTMODE_100x37:
            text_buffer_width = 100;
            text_buffer_height = 37;
            bitness = 16;
            break;
        case TEXTMODE_128x48:
            text_buffer_width = MAX_WIDTH;
            text_buffer_height = MAX_HEIGHT;
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
        case GRAPHICS_320x240x256:
            text_buffer_width = 320;
            text_buffer_height = 240;
            bitness = 8;
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

    double fdiv = 100;
    int HS_SIZE = 4;
    int HS_SHIFT = 100;

    switch (graphics_mode) {
        case TEXTMODE_53x30:
        case TEXTMODE_80x30:
        case GRAPHICS_320x240x256:
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
            set_vga_clkdiv(25175000, line_size); // частота пиксельклока
            break;
        case TEXTMODE_100x37:
            TMPL_LINE8 = 0b11000000;
            // SVGA Signal 800 x 600 @ 60 Hz timing
            HS_SHIFT = 800 + 40; // Front porch + Visible area
            HS_SIZE = 88; // Back porch
            line_size = 1056;
            shift_picture = line_size - HS_SHIFT;
            visible_line_size = 800 / 2;
            N_lines_visible = 16 * 37; // 592 < 600
            line_VS_begin = 600 + 1; // + Front porch
            line_VS_end = 600 + 3 + 4; // ++ Sync pulse 2?
            N_lines_total = 628; // Whole frame
            set_vga_clkdiv(40000000, line_size); // частота пиксельклока 40.0 MHz
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
            visible_line_size = 1024 / 2;
            N_lines_visible = 768;
            line_VS_begin = 768 + 3; // + Front porch
            line_VS_end = 768 + 3 + 6; // ++ Sync pulse 2?
            N_lines_total = 806; // Whole frame
            set_vga_clkdiv(65000000, line_size); // частота пиксельклока 65.0 MHz
            break;
    }

    //инициализация шаблонов строк и синхросигнала
    context->lines_pattern_data = (uint32_t *)pvPortCalloc(line_size, sizeof(uint32_t));;
    for (int i = 0; i < 4; i++) {
        lines_pattern[i] = &context->lines_pattern_data[i * (line_size >> 2)];
    }
    TMPL_VHS8 = TMPL_LINE8 ^ 0b11000000;
    TMPL_VS8 = TMPL_LINE8 ^ 0b10000000;
    TMPL_HS8 = TMPL_LINE8 ^ 0b01000000;
    uint8_t* base_ptr = (uint8_t *)lines_pattern[0];
    // пустая строка
    memset(base_ptr, TMPL_LINE8, line_size);
    // выровненная синхра вначале
    memset(base_ptr, TMPL_HS8, HS_SIZE);
    // кадровая синхра
    base_ptr = (uint8_t *)lines_pattern[1];
    memset(base_ptr, TMPL_VS8, line_size);
    // выровненная синхра вначале
    memset(base_ptr, TMPL_VHS8, HS_SIZE);
    // заготовки для строк с изображением
    base_ptr = (uint8_t *)lines_pattern[2];
    memcpy(base_ptr, lines_pattern[0], line_size);
    base_ptr = (uint8_t *)lines_pattern[3];
    memcpy(base_ptr, lines_pattern[0], line_size);

    frame_number = 0;
    screen_line = 0;
    input_buffer = NULL;
    prev_output_buffer = 0;

    vga_context_t* cleanup = vga_context;
    switch (mode) {
        case TEXTMODE_53x30:
        case TEXTMODE_80x30:
        case TEXTMODE_100x37:
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
        case GRAPHICS_320x240x256:
            context->graphics_buffer = lock_buffer ? vga_context->graphics_buffer : pvPortCalloc(320, 240);
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

void graphics_set_page(uint8_t* buffer, uint8_t pallette_idx) {
    if (!vga_context) return;
  //  g_conf.v_buff_offset = buffer - RAM;
    vga_context->graphics_buffer = buffer;
    g_conf.graphics_pallette_idx = pallette_idx;
};

void graphics_set_pallette_idx(uint8_t pallette_idx) {
    g_conf.graphics_pallette_idx = pallette_idx;
};

void vga_set_offset(int x, int y) {
    graphics_buffer_shift_x = x;
    graphics_buffer_shift_y = y;
};

void vga_set_flashmode(bool flash_line, bool flash_frame) {
    is_flash_frame = flash_frame;
    is_flash_line = flash_line;
};

void vga_clr_scr(uint8_t color) {
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

void vga_set_bgcolor(const uint32_t color888) {
    const uint8_t conv0[] = { 0b00, 0b00, 0b01, 0b10, 0b10, 0b10, 0b11, 0b11 };
    const uint8_t conv1[] = { 0b00, 0b01, 0b01, 0b01, 0b10, 0b11, 0b11, 0b11 };

    const uint8_t b = (color888 & 0xff) / 42;

    const uint8_t r = (color888 >> 16 & 0xff) / 42;
    const uint8_t g = (color888 >> 8 & 0xff) / 42;

    const uint8_t c_hi = conv0[r] << 4 | conv0[g] << 2 | conv0[b];
    const uint8_t c_lo = conv1[r] << 4 | conv1[g] << 2 | conv1[b];
    bg_color[0] = ((c_hi << 8 | c_lo) & 0x3f3f | palette16_mask) << 16 |
                  ((c_hi << 8 | c_lo) & 0x3f3f | palette16_mask);
    bg_color[1] = ((c_lo << 8 | c_hi) & 0x3f3f | palette16_mask) << 16 |
                  ((c_lo << 8 | c_hi) & 0x3f3f | palette16_mask);
}

void vga_driver_init(void) {
    set_vga_dma_handler_impl(dma_handler_VGA_impl);
    vga_set_bgcolor(0x000000);
    init_palette();
}
