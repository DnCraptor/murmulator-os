#include "vga.h"
#include "hardware/clocks.h"
#include "stdbool.h"
#include "hardware/structs/pll.h"
#include "hardware/structs/systick.h"

#include "hardware/dma.h"
#include "hardware/irq.h"
#include <string.h>
#include <stdio.h>
#include "hardware/pio.h"
#include "pico/stdlib.h"
#include "stdlib.h"
#include "fnt8x16.h"
//#include "pico-vision.h"
//#include "config_em.h"
#include "ram_page.h"
#include "FreeRTOS.h"
#include "task.h"

#define PIO_VGA (pio0)
#define beginVGA_PIN (6)
#define VGA_DMA_IRQ (DMA_IRQ_0)

enum graphics_mode_t {
    TEXTMODE_80x30,
    TEXTMODE_128x48,
    BK_256x256x2,
    BK_512x256x1,
};

int pallete_mask = 3; // 11 - 2 bits

uint16_t pio_program_VGA_instructions[] = {
    //     .wrap_target
    0x6008, //  0: out    pins, 8
    //     .wrap
};

const struct pio_program pio_program_VGA = {
    .instructions = pio_program_VGA_instructions,
    .length = 1,
    .origin = -1,
};

typedef struct {
    uint8_t* graphics_buffer;
} vga_context_t;

static vga_context_t* vga_context = NULL;

static uint32_t* lines_pattern[4];
static uint32_t* lines_pattern_data = NULL;
static int _SM_VGA = -1;


static volatile int N_lines_total = 525;
static volatile int N_lines_visible = 480;
static volatile int line_VS_begin = 490;
static volatile int line_VS_end = 491;
static volatile int shift_picture = 0;

static volatile int begin_line_index = 0;
static volatile int visible_line_size = 320;

static int dma_chan_ctrl;
static int dma_chan;

static volatile uint graphics_buffer_width = 0;
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

static volatile int __scratch_y("vga_driver_text") pos_x = 0;
static volatile int __scratch_y("vga_driver_text") pos_y = 0;
static volatile bool __scratch_y("vga_driver_text") cursor_blink_state = true;

static uint8_t __scratch_y("vga_driver_text") con_color = 7;
static uint8_t __scratch_y("vga_driver_text") con_bgcolor = 0;
static volatile bool lock_buffer = false;

//буфер 2К текстовой палитры для быстрой работы
static uint16_t* txt_palette_fast = NULL;
//static uint16_t txt_palette_fast[256*4];

static volatile enum graphics_mode_t graphics_mode;

// TODO: separate header for sound mixer

// регистр "защёлка" для примитивного ковокса без буфера
volatile uint16_t true_covox = 0;
volatile uint16_t az_covox_L = 0;
volatile uint16_t az_covox_R = 0;
volatile uint16_t covox_mix = 0x0F;

typedef struct config_em {
    uint8_t graphics_pallette_idx;
    uint8_t shift_y;
    uint16_t graphics_buffer_height;
   // size_t v_buff_offset;
} config_em_t;

volatile config_em_t g_conf = { 0 };

void vga_lock_buffer(bool b) {
    lock_buffer = b;
}
void vga_set_con_color(uint8_t color, uint8_t bgcolor) {
    con_color = color;
    con_bgcolor = bgcolor;
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
void vga_draw_text(const char* string, int x, int y, uint8_t color, uint8_t bgcolor) {
    if (!vga_context || !vga_context->graphics_buffer) return;
    uint8_t* t_buf = vga_context->graphics_buffer + text_buffer_width * 2 * y + 2 * x;
    uint8_t c = (bgcolor << 4) | (color & 0xF);
    for (int xi = x; xi < text_buffer_width * 2; ++xi) {
        if (!(*string)) break;
        *t_buf++ = *string++;
        *t_buf++ = c;
    }
}
size_t vga_buffer_size() {
    switch (graphics_mode) {
        case TEXTMODE_80x30:
        case TEXTMODE_128x48:
            return text_buffer_height * text_buffer_width * 2
                + 256 * 4 * sizeof(uint16_t) + line_size * sizeof(uint32_t);
        case BK_256x256x2:
        case BK_512x256x1:
        default:
            return 512 / 8 * 256
                + 256 * 4 * sizeof(uint16_t) + line_size * sizeof(uint32_t);
    }
}
uint8_t get_vga_buffer_bitness(void) {
    return bitness;
}
void vga_cleanup(void) {
    // TODO:
}

void graphics_inc_palleter_offset() {
    g_conf.graphics_pallette_idx++;
    if (g_conf.graphics_pallette_idx > 0b1111) g_conf.graphics_pallette_idx = 0;
}

static volatile uint32_t frame_number = 0;
static volatile uint32_t screen_line = 0;
static volatile uint8_t* input_buffer = NULL;
static volatile uint32_t* * prev_output_buffer = 0;

inline static void dma_handler_VGA_impl() {
    dma_hw->ints0 = 1u << dma_chan_ctrl;
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

        //синхросигналы
        if ((screen_line >= line_VS_begin) && (screen_line <= line_VS_end))
            dma_channel_set_read_addr(dma_chan_ctrl, &lines_pattern[1], false); //VS SYNC
        else
            dma_channel_set_read_addr(dma_chan_ctrl, &lines_pattern[0], false);
        return;
    }

    if (!(input_buffer)) {
        dma_channel_set_read_addr(dma_chan_ctrl, &lines_pattern[0], false);
        return;
    } //если нет видеобуфера - рисуем пустую строку

    int y, line_number;

    uint32_t* * output_buffer = &lines_pattern[2 + (screen_line & 1)];
    uint div_factor = 2;
    switch (graphics_mode) {
        case BK_256x256x2:
        case BK_512x256x1:
            if (screen_line % 3 != 0) { // три подряд строки рисуем одно и тоже
                if (prev_output_buffer) output_buffer = prev_output_buffer;
                dma_channel_set_read_addr(dma_chan_ctrl, output_buffer, false);
                return;
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
            dma_channel_set_read_addr(dma_chan_ctrl, output_buffer, false);
            return;
        }
        case TEXTMODE_128x48: {
            register uint16_t* output_buffer_16bit = (uint16_t *)*output_buffer;
            output_buffer_16bit += shift_picture / 2;
            const uint font_weight = 8;
            const uint font_height = 16;
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
            dma_channel_set_read_addr(dma_chan_ctrl, output_buffer, false);
            return;
        }
        default: {
            dma_channel_set_read_addr(dma_chan_ctrl, &lines_pattern[0], false); // TODO: ensue it is required
            return;
        }
    }
    if (y < 0) {
        dma_channel_set_read_addr(dma_chan_ctrl, &lines_pattern[0], false); // TODO: ensue it is required
        return;
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
        dma_channel_set_read_addr(dma_chan_ctrl, output_buffer, false);
        return;
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
    dma_channel_set_read_addr(dma_chan_ctrl, output_buffer, false);
}

// to start sound later
void __not_in_flash_func(dma_handler_VGA)() {
    dma_handler_VGA_impl();
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
    if (_SM_VGA < 0) return false; // если  VGA не инициализирована -
    if (graphics_mode == mode) return true;
    vga_context_t* context = pvPortCalloc(1, sizeof(vga_context_t));
    switch (mode) {
        case TEXTMODE_80x30:
            text_buffer_width = 80;
            text_buffer_height = 30;
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
            fdiv = clock_get_hz(clk_sys) / 25175000.0; // частота пиксельклока
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
            fdiv = clock_get_hz(clk_sys) / (65000000.0); // 65.0 MHz
            break;
    }

    if (lines_pattern_data) {
        vPortFree(lines_pattern_data);
    }
    //инициализация шаблонов строк и синхросигнала
    if (mode == TEXTMODE_80x30) {
        const uint32_t div32 = (uint32_t)(fdiv * (1 << 16) + 0.0);
        PIO_VGA->sm[_SM_VGA].clkdiv = div32 & 0xfffff000; //делитель для конкретной sm
        dma_channel_set_trans_count(dma_chan, line_size / 4, false);
        lines_pattern_data = (uint32_t *)pvPortCalloc(line_size, sizeof(uint32_t));
        for (int i = 0; i < 4; i++) {
            lines_pattern[i] = &lines_pattern_data[i * (line_size / 4)];
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
        uint32_t div32 = (uint32_t)(fdiv * (1 << 16) + 0.0);
        PIO_VGA->sm[_SM_VGA].clkdiv = div32 & 0xfffff000; //делитель для конкретной sm
        dma_channel_set_trans_count(dma_chan, line_size >> 2, false);
        lines_pattern_data = (uint32_t *)pvPortCalloc(line_size, sizeof(uint32_t));;
        for (int i = 0; i < 4; i++) {
            lines_pattern[i] = &lines_pattern_data[i * (line_size >> 2)];
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
            context->graphics_buffer = pvPortCalloc(text_buffer_width * text_buffer_height, 2);
            break;
        case BK_256x256x2:
        case BK_512x256x1:
            context->graphics_buffer = pvPortCalloc(512 >> 3, 256);
            break;
    }
    vga_context = context;
    if (cleanup->graphics_buffer) {
        vPortFree(cleanup->graphics_buffer);
        vPortFree(cleanup);
    }
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

void graphics_shift_screen(uint16_t Word) {
    g_conf.shift_y = Word & 0b11111111;
    // Разряд 9 - при записи “1” в этот разряд на экране отображается весь буфер экрана (256 телевизионных строк).
    // При нулевом значении в верхней части растра отображается 1/4 часть (старшие адреса) экранного ОЗУ,
    // нижняя часть экрана не отображается. Данный режим не используется базовой операционной системой.
    g_conf.graphics_buffer_height = (Word & 0b01000000000) ? 256 : 256 / 4;
}

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
    graphics_set_con_pos(0, 0);
    graphics_set_con_color(7, color); // TODO:
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
    if (!(txt_palette_fast)) {
        txt_palette_fast = (uint16_t *)pvPortCalloc(256 * 4, sizeof(uint16_t));
        for (int i = 0; i < 256; i++) {
            uint8_t c1 = txt_palette[i & 0xf];
            uint8_t c0 = txt_palette[i >> 4];
            txt_palette_fast[i * 4 + 0] = (c0) | (c0 << 8);
            txt_palette_fast[i * 4 + 1] = (c1) | (c0 << 8);
            txt_palette_fast[i * 4 + 2] = (c0) | (c1 << 8);
            txt_palette_fast[i * 4 + 3] = (c1) | (c1 << 8);
        }
    }
    palette16_mask = 0xc0c0;
    //корректировка  палитры по маске бит синхры
    bg_color[0] = (bg_color[0] & 0x3f3f3f3f) | palette16_mask | (palette16_mask << 16);
    bg_color[1] = (bg_color[1] & 0x3f3f3f3f) | palette16_mask | (palette16_mask << 16);
    for (int i = 0; i < 16*4; i++) {
        palette[i] = (palette[i] & 0x3f3f) | palette16_mask;
    }
}

void vga_init() {
    if (vga_context) {
        return;
    };
    vga_context = pvPortCalloc(1, sizeof(vga_context_t));
    vga_context->graphics_buffer = pvPortCalloc(MAX_WIDTH * MAX_HEIGHT, 2);
    text_buffer_width = MAX_WIDTH;
    text_buffer_height = MAX_HEIGHT;
    vga_set_bgcolor(0x000000);
    init_palette();
    //инициализация PIO
    //загрузка программы в один из PIO
    uint offset = pio_add_program(PIO_VGA, &pio_program_VGA);
    _SM_VGA = pio_claim_unused_sm(PIO_VGA, true);
    uint sm = _SM_VGA;
    for (int i = 0; i < 8; i++) {
        gpio_init(beginVGA_PIN + i);
        gpio_set_dir(beginVGA_PIN + i, GPIO_OUT);
        pio_gpio_init(PIO_VGA, beginVGA_PIN + i);
    }; //резервируем под выход PIO
    pio_sm_set_consecutive_pindirs(PIO_VGA, sm, beginVGA_PIN, 8, true); //конфигурация пинов на выход
    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_wrap(&c, offset + 0, offset + (pio_program_VGA.length - 1));
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX); //увеличение буфера TX за счёт RX до 8-ми
    sm_config_set_out_shift(&c, true, true, 32);
    sm_config_set_out_pins(&c, beginVGA_PIN, 8);
    pio_sm_init(PIO_VGA, sm, offset, &c);
    pio_sm_set_enabled(PIO_VGA, sm, true);
    //инициализация DMA
    dma_chan_ctrl = dma_claim_unused_channel(true);
    dma_chan = dma_claim_unused_channel(true);
    //основной ДМА канал для данных
    dma_channel_config c0 = dma_channel_get_default_config(dma_chan);
    channel_config_set_transfer_data_size(&c0, DMA_SIZE_32);
    channel_config_set_read_increment(&c0, true);
    channel_config_set_write_increment(&c0, false);
    uint dreq = DREQ_PIO1_TX0 + sm;
    if (PIO_VGA == pio0) dreq = DREQ_PIO0_TX0 + sm;
    channel_config_set_dreq(&c0, dreq);
    channel_config_set_chain_to(&c0, dma_chan_ctrl); // chain to other channel
    dma_channel_configure(
        dma_chan,
        &c0,
        &PIO_VGA->txf[sm], // Write address
        lines_pattern[0], // read address
        600 / 4, //
        false // Don't start yet
    );
    //канал DMA для контроля основного канала
    dma_channel_config c1 = dma_channel_get_default_config(dma_chan_ctrl);
    channel_config_set_transfer_data_size(&c1, DMA_SIZE_32);
    channel_config_set_read_increment(&c1, false);
    channel_config_set_write_increment(&c1, false);
    channel_config_set_chain_to(&c1, dma_chan); // chain to other channel
    dma_channel_configure(
        dma_chan_ctrl,
        &c1,
        &dma_hw->ch[dma_chan].read_addr, // Write address
        &lines_pattern[0], // read address
        1, //
        false // Don't start yet
    );
    vga_set_mode(BK_256x256x2);
    irq_set_exclusive_handler(VGA_DMA_IRQ, dma_handler_VGA);
    dma_channel_set_irq0_enabled(dma_chan_ctrl, true);
    irq_set_enabled(VGA_DMA_IRQ, true);
    dma_start_channel_mask((1u << dma_chan));
    vga_set_mode(TEXTMODE_128x48);
};

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

void vga_print(char* buf) {
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

void vga_backspace(void) {
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
