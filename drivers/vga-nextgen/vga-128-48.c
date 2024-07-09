// TODO: merge VGA drv to one file
#include "graphics.h"
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

int __scratch_y("vga_driver_text") pallete_mask = 3; // 11 - 2 bits

static uint32_t* __scratch_y("vga_driver_text") lines_pattern[4]; // TODO: unify
static uint32_t* __scratch_y("vga_driver_text") lines_pattern_data = NULL;
static int __scratch_y("vga_driver_text") _SM_VGA = -1;

#define TMPL_LINE8 0b11000000
// XGA Signal 1024 x 768 @ 60 Hz timing
#define HS_SHIFT (1024 + 24) // Front porch + Visible area
#define HS_SIZE 160 // Back porch
#define line_size 1344
#define shift_picture (line_size - HS_SHIFT)
#define palette16_mask 0xC0C0
#define visible_line_size (1024 / 2)
#define N_lines_visible 768
#define line_VS_begin (768 + 3) // + Front porch
#define line_VS_end (768 + 3 + 6) // ++ Sync pulse 2?
#define N_lines_total 806 // Whole frame

static int __scratch_y("vga_driver_text") begin_line_index = 0;
static int __scratch_y("vga_driver_text") dma_chan_ctrl;
static int __scratch_y("vga_driver_text") dma_chan;

static uint8_t* __scratch_y("vga_driver_text") text_buffer;

static bool __scratch_y("vga_driver_text") is_flash_line = true;
static bool __scratch_y("vga_driver_text") is_flash_frame = true;

//буфер 1к графической палитры
static uint16_t __scratch_y("vga_driver_text") palette[16*4];
static uint __scratch_y("vga_driver_text") text_buffer_width = 0;
static uint __scratch_y("vga_driver_text") text_buffer_height = 0;
static size_t __scratch_y("vga_driver_text") text_buffer_size = 0;

static uint32_t __scratch_y("vga_driver_text") bg_color[2];
static volatile int __scratch_y("vga_driver_text") pos_x = 0;
static volatile int __scratch_y("vga_driver_text") pos_y = 0;
static volatile bool __scratch_y("vga_driver_text") cursor_blink_state = true;

static uint8_t __scratch_y("vga_driver_text") con_color = 7;
static uint8_t __scratch_y("vga_driver_text") con_bgcolor = 0;
static uint16_t __scratch_y("vga_driver_text") txt_palette[16];

//буфер 2К текстовой палитры для быстрой работы
static uint16_t* __scratch_y("vga_driver_text") txt_palette_fast = NULL;
//static uint16_t txt_palette_fast[256*4];

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
    return text_buffer;
}
void set_vga_buffer(uint8_t* buffer) {
    text_buffer = buffer;
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
void vga_clr_scr(const uint8_t color) {
    if (!text_buffer) return;
    uint16_t* t_buf = (uint16_t *)text_buffer;
    int size = text_buffer_width * text_buffer_height;
    while (size--) *t_buf++ = color << 4 | ' ';
    graphics_set_con_pos(0, 0);
    graphics_set_con_color(7, color); // TODO:
}
void vga_draw_text(const char* string, int x, int y, uint8_t color, uint8_t bgcolor) {
    uint8_t* t_buf = text_buffer + text_buffer_width * 2 * y + 2 * x;
    uint8_t c = (bgcolor << 4) | (color & 0xF);
    for (int xi = x; xi < text_buffer_width * 2; ++xi) {
        if (!(*string)) break;
        *t_buf++ = *string++;
        *t_buf++ = c;
    }
}
size_t vga_buffer_size() {
    return text_buffer_size;
}
uint8_t get_vga_buffer_bitness(void) {
    return 16;
}
void vga_cleanup(void) {
}
// TODO: unify
static volatile uint8_t graphics_pallette_idx = 0;
uint8_t get_graphics_pallette_idx() {
    return graphics_pallette_idx;
}
void graphics_set_pallette_idx(uint8_t pallette_idx) {
    graphics_pallette_idx = pallette_idx;
};

void __scratch_y("vga_driver") dma_handler_VGA() {
    dma_hw->ints0 = 1u << dma_chan_ctrl;
    static uint32_t frame_number = 0;
    static uint32_t screen_line = 0;
    static uint8_t* input_buffer = NULL;
    static uint32_t* * prev_output_buffer = 0;
    screen_line++;

    if (screen_line == N_lines_total) {
        screen_line = 0;
        frame_number++;
        input_buffer = text_buffer;
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
    uint16_t* output_buffer_16bit = (uint16_t *)*output_buffer;
    output_buffer_16bit += shift_picture / 2;
    const uint font_weight = 8;
    const uint font_height = 16;
    // "слой" символа
    uint32_t glyph_line = screen_line % font_height;
    //указатель откуда начать считывать символы
    uint8_t* text_buffer_line = &text_buffer[screen_line / font_height * text_buffer_width * 2];
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
        }
        else {
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

#define TEXTMODE_COLS (128)
#define TEXTMODE_ROWS (48)

static void vga_init_palette() {
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
};

void vga_init(void) {
    // Если мы уже проиницилизированы - выходим
    if (text_buffer && (txt_palette_fast) && (lines_pattern_data)) {
        return;
    };
    text_buffer_size = TEXTMODE_COLS * TEXTMODE_ROWS * 2;
    text_buffer = (uint8_t*)pvPortMalloc(text_buffer_size);
    text_buffer_width = TEXTMODE_COLS;
    text_buffer_height = TEXTMODE_ROWS;
    vga_set_bgcolor(0x000000);

    uint8_t TMPL_VHS8 = 0;
    uint8_t TMPL_VS8 = 0;
    uint8_t TMPL_HS8 = 0;
    vga_init_palette();

    //корректировка  палитры по маске бит синхры
    bg_color[0] = (bg_color[0] & 0x3f3f3f3f) | palette16_mask | (palette16_mask << 16);
    bg_color[1] = (bg_color[1] & 0x3f3f3f3f) | palette16_mask | (palette16_mask << 16);
    for (int i = 0; i < 16*4; i++) {
        palette[i] = (palette[i] & 0x3f3f) | palette16_mask;
    }
        //инициализация PIO
    //загрузка программы в один из PIO
    const uint offset = pio_add_program(PIO_VGA, &pio_program_VGA);
    _SM_VGA = pio_claim_unused_sm(PIO_VGA, true);
    const uint sm = _SM_VGA;

    for (int i = 0; i < 8; i++) {
        gpio_init(VGA_BASE_PIN + i);
        gpio_set_dir(VGA_BASE_PIN + i, GPIO_OUT);
        pio_gpio_init(PIO_VGA, VGA_BASE_PIN + i);
    }; //резервируем под выход PIO

    //pio_sm_config c = pio_vga_program_get_default_config(offset);

    pio_sm_set_consecutive_pindirs(PIO_VGA, sm, VGA_BASE_PIN, 8, true); //конфигурация пинов на выход

    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_wrap(&c, offset + 0, offset + (pio_program_VGA.length - 1));

    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX); //увеличение буфера TX за счёт RX до 8-ми
    sm_config_set_out_shift(&c, true, true, 32);
    sm_config_set_out_pins(&c, VGA_BASE_PIN, 8);
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
    //channel_config_set_dreq(&c1, DREQ_PIO0_TX0);

    dma_channel_configure(
        dma_chan_ctrl,
        &c1,
        &dma_hw->ch[dma_chan].read_addr, // Write address
        &lines_pattern[0], // read address
        1, //
        false // Don't start yet
    );

    double fdiv = clock_get_hz(clk_sys) / (65000000.0); // 65.0 MHz
    //инициализация шаблонов строк и синхросигнала
    if (!(lines_pattern_data)) //выделение памяти, если не выделено
    {
        uint32_t div32 = (uint32_t)(fdiv * (1 << 16) + 0.0);
        PIO_VGA->sm[_SM_VGA].clkdiv = div32 & 0xfffff000; //делитель для конкретной sm
        dma_channel_set_trans_count(dma_chan, line_size / 4, false);

        lines_pattern_data = (uint32_t *)pvPortCalloc(line_size * 4 / 4, sizeof(uint32_t));;

        for (int i = 0; i < 4; i++) {
            lines_pattern[i] = &lines_pattern_data[i * (line_size / 4)];
        }
        // memset(lines_pattern_data,N_TMPLS*1200,0);
        TMPL_VHS8 = TMPL_LINE8 ^ 0b11000000;
        TMPL_VS8 = TMPL_LINE8 ^ 0b10000000;
        TMPL_HS8 = TMPL_LINE8 ^ 0b01000000;

        uint8_t* base_ptr = (uint8_t *)lines_pattern[0];
        //пустая строка
        memset(base_ptr, TMPL_LINE8, line_size);
        //memset(base_ptr+HS_SHIFT,TMPL_HS8,HS_SIZE);
        //выровненная синхра вначале
        memset(base_ptr, TMPL_HS8, HS_SIZE);

        // кадровая синхра
        base_ptr = (uint8_t *)lines_pattern[1];
        memset(base_ptr, TMPL_VS8, line_size);
        //memset(base_ptr+HS_SHIFT,TMPL_VHS8,HS_SIZE);
        //выровненная синхра вначале
        memset(base_ptr, TMPL_VHS8, HS_SIZE);

        //заготовки для строк с изображением
        base_ptr = (uint8_t *)lines_pattern[2];
        memcpy(base_ptr, lines_pattern[0], line_size);
        base_ptr = (uint8_t *)lines_pattern[3];
        memcpy(base_ptr, lines_pattern[0], line_size);
    }

    irq_set_exclusive_handler(VGA_DMA_IRQ, dma_handler_VGA);

    dma_channel_set_irq0_enabled(dma_chan_ctrl, true);

    irq_set_enabled(VGA_DMA_IRQ, true);
    dma_start_channel_mask(1u << dma_chan);
    for (int i = 0; i < 16; i++) {
        txt_palette[i] = (txt_palette[i] & 0x3f) | (palette16_mask >> 8);
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

};
/* TODO:
void graphics_set_page(uint8_t* buffer, uint8_t pallette_idx) {
    g_conf.v_buff_offset = buffer - RAM;
    graphics_buffer = buffer;
    g_conf.graphics_pallette_idx = pallette_idx;
};

void graphics_shift_screen(uint16_t Word) {
    g_conf.shift_y = Word & 0b11111111;
    // Разряд 9 - при записи “1” в этот разряд на экране отображается весь буфер экрана (256 телевизионных строк).
    // При нулевом значении в верхней части растра отображается 1/4 часть (старшие адреса) экранного ОЗУ,
    // нижняя часть экрана не отображается. Данный режим не используется базовой операционной системой.
    g_conf.graphics_buffer_height = (Word & 0b01000000000) ? 256 : 256 / 4;
}

void graphics_set_buffer(uint8_t* buffer, uint16_t width, uint16_t height) {
    g_conf.v_buff_offset = buffer - RAM;
    graphics_buffer = buffer;
    graphics_buffer_width = width;
    g_conf.graphics_buffer_height = height;
};

static int current_line = 25;
static int start_debug_line = 25;

void clrScr(uint8_t color) {
    uint8_t* t_buf = text_buffer;
    for (int yi = start_debug_line; yi < text_buffer_height; yi++)
        for (int xi = 0; xi < text_buffer_width * 2; xi++) {
            *t_buf++ = ' ';
            *t_buf++ = (color << 4) | (color & 0xF);
        }
    current_line = start_debug_line;
};

void set_start_debug_line(int _start_debug_line) {
    start_debug_line = _start_debug_line;
}
*/


#ifdef SAVE_VIDEO_RAM_ON_MANAGER
#include "emulator.h"
static FATFS fs;
static FIL file;
static int video_ram_level = 0;

bool save_video_ram() {
    gpio_put(PICO_DEFAULT_LED_PIN, true);
    char path[16];
    sprintf(path, "\\BK\\video%d.ram", video_ram_level);
    FRESULT result = f_open(&file, path, FA_WRITE | FA_CREATE_ALWAYS);
    if (result != FR_OK) {
        return false;
    }
    UINT bw;
    result = f_write(&file, TEXT_VIDEO_RAM, sizeof(TEXT_VIDEO_RAM), &bw);
    if (result != FR_OK) {
        return false;
    }
    f_close(&file);
    video_ram_level++;
    gpio_put(PICO_DEFAULT_LED_PIN, false);
    return true;
}
bool restore_video_ram() {
    gpio_put(PICO_DEFAULT_LED_PIN, true);
    video_ram_level--;
    char path[16];
    sprintf(path, "\\BK\\video%d.ram", video_ram_level);
    FRESULT result = f_open(&file, path, FA_READ);
    if (result == FR_OK) {
      UINT bw;
      result = f_read(&file, TEXT_VIDEO_RAM, sizeof(TEXT_VIDEO_RAM), &bw);
      if (result != FR_OK) {
        return false;
      }
    }
    f_close(&file);
    f_unlink(path);
    gpio_put(PICO_DEFAULT_LED_PIN, false);
    return true;
}
#else
bool save_video_ram() {}
bool restore_video_ram() {}
#endif

static char* _rollup(char* t_buf) {
    if (pos_y >= text_buffer_height - 1) {
        memcpy(text_buffer, text_buffer + text_buffer_width * 2, text_buffer_width * (text_buffer_height - 2) * 2);
        t_buf = text_buffer + text_buffer_width * (text_buffer_height - 2) * 2;
        for(int i = 0; i < text_buffer_width; ++i) {
            *t_buf++ = ' ';
            *t_buf++ = con_bgcolor << 4 | con_color & 0xF;
        }
        pos_y = text_buffer_height - 2;
    }
    return text_buffer + text_buffer_width * 2 * pos_y + 2 * pos_x;
}

void vga_print(char* buf) {
    uint8_t* t_buf = text_buffer + text_buffer_width * 2 * pos_y + 2 * pos_x;
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
    uint8_t* t_buf;
    pos_x--;
    if (pos_x < 0) {
        pos_x = text_buffer_width - 2;
        pos_y--;
        if (pos_y < 0) {
            pos_y = 0;
        }
    }
    t_buf = text_buffer + text_buffer_width * 2 * pos_y + 2 * pos_x;
    *t_buf++ = ' ';
    *t_buf++ = con_bgcolor << 4 | con_color & 0xF;
}

int vga_get_mode(void) {
    return 0;
}

bool vga_set_mode(int mode) {
    return mode == 0;
}

void vga_lock_buffer(bool b) {
    // TODO:
}

bool vga_is_text_mode() {
    return true;
}

bool vga_is_mode_text(int mode) {
    return true;
}
