// TODO: external driver
#include "graphics.h"
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

#include "FreeRTOS.h"
#include "task.h"

#ifndef logMsg
   
#ifdef DEBUG_VGA
char __scratch_y("vga_driver_text") vga_dbg_msg[1024] = { 0 };
#define DBG_PRINT(...) { sprintf(vga_dbg_msg + strlen(vga_dbg_msg), __VA_ARGS__); }
#else
#define DBG_PRINT(...)
#endif

#endif


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


static uint32_t* __scratch_y("vga_driver_text") lines_pattern[4];
static uint32_t* __scratch_y("vga_driver_text") lines_pattern_data = NULL;
static int __scratch_y("vga_driver_text") _SM_VGA = -1;

static int __scratch_y("vga_driver_text") N_lines_total = 525;
static int __scratch_y("vga_driver_text") N_lines_visible = 480;
static int __scratch_y("vga_driver_text") line_VS_begin = 490;
static int __scratch_y("vga_driver_text") line_VS_end = 491;
static int __scratch_y("vga_driver_text") shift_picture = 0;
static int __scratch_y("vga_driver_text") visible_line_size = 320;

static int __scratch_y("vga_driver_text") dma_chan_ctrl;
static int __scratch_y("vga_driver_text") dma_chan;

static uint8_t* __scratch_y("vga_driver_text") text_buffer = 0;
static uint __scratch_y("vga_driver_text") text_buffer_width = 0;
static uint __scratch_y("vga_driver_text") text_buffer_height = 0;
static size_t __scratch_y("vga_driver_text") text_buffer_size = 0;

//буфер 1к графической палитры
static uint16_t __scratch_y("vga_driver_text") palette[2][256];
static uint32_t __scratch_y("vga_driver_text") bg_color[2];
static uint16_t __scratch_y("vga_driver_text") palette16_mask = 0;
static uint16_t __scratch_y("vga_driver_text") txt_palette[16];

//буфер 2К текстовой палитры для быстрой работы
static uint16_t* __scratch_y("vga_driver_text") txt_palette_fast = NULL;
static volatile __scratch_y("vga_driver_text") int pos_x = 0;
static volatile __scratch_y("vga_driver_text") int pos_y = 0;
static volatile __scratch_y("vga_driver_text") bool cursor_blink_state = true;

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
    DBG_PRINT("vga_cleanup enter\n");
    if ( _SM_VGA < 0 ) return;
    
    dma_channel_set_irq0_enabled(dma_chan_ctrl, false);
    
    irq_set_enabled(VGA_DMA_IRQ, false);
    irq_remove_handler(VGA_DMA_IRQ, irq_get_exclusive_handler(VGA_DMA_IRQ));

    //остановка всех каналов DMA
    dma_hw->abort = (1 << dma_chan_ctrl) | (1 << dma_chan);
    while (dma_hw->abort) tight_loop_contents();
    dma_channel_unclaim(dma_chan);
    dma_channel_unclaim(dma_chan_ctrl);

    for (int i = 0; i < 8; i++) {
        gpio_deinit(VGA_BASE_PIN + i);
    };
    pio_sm_set_enabled(PIO_VGA, _SM_VGA, false);
    pio_sm_unclaim(PIO_VGA, _SM_VGA);
    _SM_VGA = -1;
    pio_clear_instruction_memory(PIO_VGA);

    vPortFree(txt_palette_fast);
    vPortFree(lines_pattern_data);
    vPortFree(text_buffer);
    text_buffer = 0;
    txt_palette_fast = 0;
    lines_pattern_data = 0;
    DBG_PRINT("vga_cleanup exit\n");
}

void __scratch_y("vga_driver") dma_handler_VGA() {
    dma_hw->ints0 = 1u << dma_chan_ctrl;
    static uint32_t __scratch_y("vga_driver_text") frame_number = 0;
    static uint32_t __scratch_y("vga_driver_text") screen_line = 0;
    static uint8_t* __scratch_y("vga_driver_text") input_buffer = NULL;
    screen_line++;
    if (screen_line == N_lines_total) {
        screen_line = 0;
        frame_number++;
        input_buffer = text_buffer;
    }

    if (screen_line >= N_lines_visible) {
        //заполнение цветом фона
        if (screen_line == N_lines_visible | screen_line == N_lines_visible + 3) {
            uint32_t* output_buffer_32bit = lines_pattern[2 + (screen_line & 1)];
            output_buffer_32bit += shift_picture / 4;
            uint32_t color32 = bg_color[0];
            for (int i = visible_line_size / 2; i--;) {
                *output_buffer_32bit++ = color32;
            }
        }

        //синхросигналы
        if (screen_line >= line_VS_begin && screen_line <= line_VS_end)
            dma_channel_set_read_addr(dma_chan_ctrl, &lines_pattern[1], false); //VS SYNC
        else
            dma_channel_set_read_addr(dma_chan_ctrl, &lines_pattern[0], false);
        return;
    }

    if (!input_buffer) {
        dma_channel_set_read_addr(dma_chan_ctrl, &lines_pattern[0], false);
        return;
    } //если нет видеобуфера - рисуем пустую строку

    int y, line_number;

    uint32_t* * output_buffer = &lines_pattern[2 + (screen_line & 1)];
    uint16_t* output_buffer_16bit = (uint16_t *)*output_buffer;
    output_buffer_16bit += shift_picture / 2;
    const uint font_height = 16;
    // "слой" символа
    uint32_t glyph_line = screen_line % font_height;
    //указатель откуда начать считывать символы
    uint8_t* text_buffer_line = &text_buffer[screen_line / font_height * text_buffer_width * 2];
    for (int x = 0; x < text_buffer_width; x++) {
        //из таблицы символов получаем "срез" текущего символа
        uint8_t glyph_pixels = font_8x16[*text_buffer_line++ * font_height + glyph_line];
        //считываем из быстрой палитры начало таблицы быстрого преобразования 2-битных комбинаций цветов пикселей
        uint16_t* color = &txt_palette_fast[*text_buffer_line++ * 4];
#if 1
        if (cursor_blink_state && (screen_line >> 4) == pos_y && x == pos_x && glyph_line >= 13) { // TODO: cur height
            color = &txt_palette_fast[0];
            uint16_t c = color[7]; // TODO: setup cursor color
            *output_buffer_16bit++ = c;
            *output_buffer_16bit++ = c;
            *output_buffer_16bit++ = c;
            *output_buffer_16bit++ = c;
            if (text_buffer_width == 40) {
                *output_buffer_16bit++ = c;
                *output_buffer_16bit++ = c;
                *output_buffer_16bit++ = c;
                *output_buffer_16bit++ = c;
            }
        }
        else
#endif
        {
            *output_buffer_16bit++ = color[glyph_pixels & 3];
            if (text_buffer_width == 40) *output_buffer_16bit++ = color[glyph_pixels & 3];
            glyph_pixels >>= 2;
            *output_buffer_16bit++ = color[glyph_pixels & 3];
            if (text_buffer_width == 40) *output_buffer_16bit++ = color[glyph_pixels & 3];
            glyph_pixels >>= 2;
            *output_buffer_16bit++ = color[glyph_pixels & 3];
            if (text_buffer_width == 40) *output_buffer_16bit++ = color[glyph_pixels & 3];
            glyph_pixels >>= 2;
            *output_buffer_16bit++ = color[glyph_pixels & 3];
            if (text_buffer_width == 40) *output_buffer_16bit++ = color[glyph_pixels & 3];
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

#define TEXTMODE_COLS (80)
#define TEXTMODE_ROWS (30)

void vga_init(void) {
    DBG_PRINT("vga_init enter\n");
    text_buffer_size = TEXTMODE_COLS * TEXTMODE_ROWS * 2;
    text_buffer = (uint8_t*)pvPortMalloc(text_buffer_size);
    DBG_PRINT("vga_init buffer: %ph\n", text_buffer);
    text_buffer_width = TEXTMODE_COLS;
    text_buffer_height = TEXTMODE_ROWS;
    vga_set_bgcolor(0x000000);
    DBG_PRINT("vga_init bgcolor: 0\n");
    
    //инициализация палитры по умолчанию
#if 1
    const uint8_t conv0[] = { 0b00, 0b00, 0b01, 0b10, 0b10, 0b10, 0b11, 0b11 };
    const uint8_t conv1[] = { 0b00, 0b01, 0b01, 0b01, 0b10, 0b11, 0b11, 0b11 };
    for (int i = 0; i < 256; i++) {
        const uint8_t b = i & 0b11;
        const uint8_t r = i >> 5 & 0b111;
        const uint8_t g = i >> 2 & 0b111;

        const uint8_t c_hi = 0xc0 | conv0[r] << 4 | conv0[g] << 2 | b;
        const uint8_t c_lo = 0xc0 | conv1[r] << 4 | conv1[g] << 2 | b;

        palette[0][i] = c_hi << 8 | c_lo;
        palette[1][i] = c_lo << 8 | c_hi;
    }
#endif
    //текстовая палитра
    for (int i = 0; i < 16; i++) {
        const uint8_t b = i & 1 ? (i >> 3 ? 3 : 2) : 0;
        const uint8_t r = i & 4 ? (i >> 3 ? 3 : 2) : 0;
        const uint8_t g = i & 2 ? (i >> 3 ? 3 : 2) : 0;

        const uint8_t c = r << 4 | g << 2 | b;

        txt_palette[i] = c & 0x3f | 0xc0;
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

    uint8_t TMPL_VHS8 = 0;
    uint8_t TMPL_VS8 = 0;
    uint8_t TMPL_HS8 = 0;
    uint8_t TMPL_LINE8 = 0b11000000;
    int HS_SHIFT = 328 * 2;
    int HS_SIZE = 48 * 2;
    int line_size = 400 * 2;
    shift_picture = line_size - HS_SHIFT;
    palette16_mask = 0xc0c0;
    visible_line_size = 320;
    N_lines_total = 525;
    N_lines_visible = 480;
    line_VS_begin = 490;
    line_VS_end = 491;
    double fdiv = clock_get_hz(clk_sys) / 25175000.0; //частота пиксельклока

    //корректировка  палитры по маске бит синхры
    bg_color[0] = bg_color[0] & 0x3f3f3f3f | palette16_mask | palette16_mask << 16;
    bg_color[1] = bg_color[1] & 0x3f3f3f3f | palette16_mask | palette16_mask << 16;
    for (int i = 0; i < 256; i++) {
        palette[0][i] = palette[0][i] & 0x3f3f | palette16_mask;
        palette[1][i] = palette[1][i] & 0x3f3f | palette16_mask;
    }

    //инициализация шаблонов строк и синхросигнала
    if (!lines_pattern_data) //выделение памяти, если не выделено
    {
        const uint32_t div32 = (uint32_t)(fdiv * (1 << 16) + 0.0);
        PIO_VGA->sm[_SM_VGA].clkdiv = div32 & 0xfffff000; //делитель для конкретной sm
        dma_channel_set_trans_count(dma_chan, line_size / 4, false);

        text_buffer_size += line_size * sizeof(uint32_t);
        lines_pattern_data = (uint32_t *)pvPortCalloc(line_size, sizeof(uint32_t));

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

    irq_set_exclusive_handler(VGA_DMA_IRQ, dma_handler_VGA);

    dma_channel_set_irq0_enabled(dma_chan_ctrl, true);

    irq_set_enabled(VGA_DMA_IRQ, true);
    dma_start_channel_mask(1u << dma_chan);

    //текстовая палитра
    for (int i = 0; i < 16; i++) {
        txt_palette[i] = txt_palette[i] & 0x3f | palette16_mask >> 8;
    }
    if (!txt_palette_fast) {
        text_buffer_size += 256 * 4 * sizeof(uint16_t);
        txt_palette_fast = (uint16_t *)pvPortCalloc(256 * 4, sizeof(uint16_t));
        for (int i = 0; i < 256; i++) {
            const uint8_t c1 = txt_palette[i & 0xf];
            const uint8_t c0 = txt_palette[i >> 4];
            txt_palette_fast[i * 4 + 0] = c0 | c0 << 8;
            txt_palette_fast[i * 4 + 1] = c1 | c0 << 8;
            txt_palette_fast[i * 4 + 2] = c0 | c1 << 8;
            txt_palette_fast[i * 4 + 3] = c1 | c1 << 8;
        }
    }
    DBG_PRINT("vga_init exit\n");
}
