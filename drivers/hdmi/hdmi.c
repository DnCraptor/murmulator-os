#include "graphics.h"
#include <stdio.h>
#include <string.h>
#include <stdalign.h>
#include "hardware/dma.h"
#include "hardware/pio.h"
#include "pico/time.h"
#include "pico/multicore.h"
#include "hardware/clocks.h"

//PIO параметры
static uint offs_prg0 = 0;
static uint offs_prg1 = 0;

//SM
static int SM_video = -1;
static int SM_conv = -1;

enum graphics_mode_t {
    TEXTMODE_53x30,
#if TEXTMODE_53x60
    TEXTMODE_53x60,
#endif
    TEXTMODE_80x30, // 640*480
    VGA_320x240x16
};

int hdmi_get_default_mode(void) {
    return TEXTMODE_53x30;
}

//активный видеорежим
static enum graphics_mode_t graphics_mode = TEXTMODE_80x30;

//буфер  палитры 256 цветов в формате R8G8B8
static uint32_t* __scratch_y("_driver_text") palette = NULL; // [256];

#define SCREEN_WIDTH (320)
#define SCREEN_HEIGHT (240)
//графический буфер
static uint8_t* __scratch_y("hdmi_ptr_1") graphics_buffer = NULL;
static int __scratch_y("_driver_text") graphics_buffer_width = 0;
static int __scratch_y("_driver_text") graphics_buffer_height = 0;
static int __scratch_y("_driver_text") graphics_buffer_shift_x = 0;
static int __scratch_y("_driver_text") graphics_buffer_shift_y = 0;
static uint8_t __scratch_y("_driver_text") bitness = 16;

extern volatile int pos_x;
extern volatile int pos_y;
extern volatile uint8_t font_width;
extern volatile uint8_t font_height;
extern volatile uint8_t* font_table;
extern volatile uint8_t con_color;
extern volatile uint8_t con_bgcolor;
extern volatile uint8_t _cursor_color;
extern volatile bool lock_buffer;

//DMA каналы
//каналы работы с первичным графическим буфером
static int dma_chan_ctrl;
static int dma_chan;
//каналы работы с конвертацией палитры
static int dma_chan_pal_conv_ctrl;
static int dma_chan_pal_conv;

//DMA буферы
//основные строчные данные
static uint32_t* __scratch_y("hdmi_ptr_3") dma_lines[2] = { NULL, NULL };
static uint32_t* __scratch_y("hdmi_ptr_4") DMA_BUF_ADDR[2];

//ДМА палитра для конвертации
//в хвосте этой памяти выделяется dma_data
alignas(4096) uint32_t hdmi_conv_color[1224];

//индекс, проверяющий зависание
static uint32_t irq_inx = 0;

//функции и константы HDMI

#define BASE_HDMI_CTRL_INX (240)
//программа конвертации адреса

uint16_t pio_program_instructions_conv_HDMI[] = {
    //         //     .wrap_target
    0x80a0, //  0: pull   block
    0x40e8, //  1: in     osr, 8
    0x4034, //  2: in     x, 20
    0x8020, //  3: push   block
    //     .wrap
};


const struct pio_program pio_program_conv_addr_HDMI = {
    .instructions = pio_program_instructions_conv_HDMI,
    .length = 4,
    .origin = -1,
};

//программа видеовывода
static const uint16_t instructions_PIO_HDMI[] = {
    0x7006, //  0: out    pins, 6         side 2
    0x7006, //  1: out    pins, 6         side 2
    0x7006, //  2: out    pins, 6         side 2
    0x7006, //  3: out    pins, 6         side 2
    0x7006, //  4: out    pins, 6         side 2
    0x6806, //  5: out    pins, 6         side 1
    0x6806, //  6: out    pins, 6         side 1
    0x6806, //  7: out    pins, 6         side 1
    0x6806, //  8: out    pins, 6         side 1
    0x6806, //  9: out    pins, 6         side 1
};

static const struct pio_program program_PIO_HDMI = {
    .instructions = instructions_PIO_HDMI,
    .length = 10,
    .origin = -1,
};

static uint64_t get_ser_diff_data(const uint16_t dataR, const uint16_t dataG, const uint16_t dataB) {
    uint64_t out64 = 0;
    for (int i = 0; i < 10; i++) {
        out64 <<= 6;
        if (i == 5) out64 <<= 2;
#ifdef PICO_PC
        uint8_t bG = (dataR >> (9 - i)) & 1;
        uint8_t bR = (dataG >> (9 - i)) & 1;
#else
        uint8_t bR = (dataR >> (9 - i)) & 1;
        uint8_t bG = (dataG >> (9 - i)) & 1;
#endif
        uint8_t bB = (dataB >> (9 - i)) & 1;

        bR |= (bR ^ 1) << 1;
        bG |= (bG ^ 1) << 1;
        bB |= (bB ^ 1) << 1;

        if (HDMI_PIN_invert_diffpairs) {
            bR ^= 0b11;
            bG ^= 0b11;
            bB ^= 0b11;
        }
        uint8_t d6;
        if (HDMI_PIN_RGB_notBGR) {
            d6 = (bR << 4) | (bG << 2) | (bB << 0);
        }
        else {
            d6 = (bB << 4) | (bG << 2) | (bR << 0);
        }


        out64 |= d6;
    }
    return out64;
}

//конвертор TMDS
static uint tmds_encoder(const uint8_t d8) {
    int s1 = 0;
    for (int i = 0; i < 8; i++) s1 += (d8 & (1 << i)) ? 1 : 0;
    bool is_xnor = false;
    if ((s1 > 4) || ((s1 == 4) && ((d8 & 1) == 0))) is_xnor = true;
    uint16_t d_out = d8 & 1;
    uint16_t qi = d_out;
    for (int i = 1; i < 8; i++) {
        d_out |= ((qi << 1) ^ (d8 & (1 << i))) ^ (is_xnor << i);
        qi = d_out & (1 << i);
    }

    if (is_xnor) d_out |= 1 << 9;
    else d_out |= 1 << 8;

    return d_out;
}

static void pio_set_x(PIO pio, const int sm, uint32_t v) {
    uint instr_shift = pio_encode_in(pio_x, 4);
    uint instr_mov = pio_encode_mov(pio_x, pio_isr);
    for (int i = 0; i < 8; i++) {
        const uint32_t nibble = (v >> (i * 4)) & 0xf;
        pio_sm_exec(pio, sm, pio_encode_set(pio_x, nibble));
        pio_sm_exec(pio, sm, instr_shift);
    }
    pio_sm_exec(pio, sm, instr_mov);
}

static void __scratch_y("hdmi_driver") dma_handler_HDMI() {
    static uint32_t inx_buf_dma;
    static uint line = 0;
    irq_inx++;

    dma_hw->ints0 = 1u << dma_chan_ctrl;
    dma_channel_set_read_addr(dma_chan_ctrl, &DMA_BUF_ADDR[inx_buf_dma & 1], false);

    line = line >= 524 ? 0 : line + 1;

    if ((line & 1) == 0) return;

    inx_buf_dma++;

    uint8_t* pfont_table = font_table;
    uint8_t* activ_buf = (uint8_t *)dma_lines[inx_buf_dma & 1];

    if (graphics_buffer && line < 480 ) {
        // область изображения
        auto cursor_color = _cursor_color;
        uint8_t* output_buffer = activ_buf + 72; //для выравнивания синхры;
        switch (graphics_mode) {
            case VGA_320x240x16: {
                int y = line / 2;
                // uint8_t* input_buffer = &graphics_buffer[(line / 3) * graphics_buffer_width];
                // заполняем пространство сверху и снизу графического буфера
                if (y < graphics_buffer_shift_y) {
                    memset(output_buffer, 255, SCREEN_WIDTH);
                    break;
                }
                if (false || (graphics_buffer_shift_y > y) || (y >= (graphics_buffer_shift_y + graphics_buffer_height))
                    || (graphics_buffer_shift_x >= SCREEN_WIDTH) || (
                        (graphics_buffer_shift_x + graphics_buffer_width) < 0)) {
                    memset(output_buffer, 255, SCREEN_WIDTH);
                    break;
                }
                uint8_t* activ_buf_end = output_buffer + SCREEN_WIDTH;
                // рисуем пространство слева от буфера
                for (int i = graphics_buffer_shift_x; i-- > 0;) {
                    *output_buffer++ = 255;
                }
                // рисуем сам видеобуфер+пространство справа
                uint8_t* input_buffer = &graphics_buffer[(y - graphics_buffer_shift_y) * graphics_buffer_width >> 1];
                const uint8_t* input_buffer_end = input_buffer + (graphics_buffer_width >> 1);
                if (graphics_buffer_shift_x < 0) input_buffer -= graphics_buffer_shift_x;
                auto fh = font_height;
                auto yfh = y / fh;
                if (yfh == pos_y && (y - yfh * fh) >= fh - 2) {
                    uint8_t c = textmode_palette[cursor_color];
                    register xc = pos_x;
                    register xi = 0;
                    register cntr = 0;
                    auto fw = font_width;
                    for (register int x = 0; input_buffer < input_buffer_end && x < (320 >> 1); ++x, ++cntr, ++cntr) { // 2 записи на байт
                        register uint8_t two_points = *input_buffer++; // 4 + 4 bits in input buffer
                        if (cntr == fw) {
                            ++xi; cntr = 0;
                        }
                        if (xi == xc) {
                            *output_buffer++ = c;
                            *output_buffer++ = c;
                        } else {
                            *output_buffer++ = textmode_palette[ two_points & 0x0F ];
                            *output_buffer++ = textmode_palette[ two_points >> 4 ];
                        }
                    }
                    while (activ_buf_end > output_buffer) {
                        *output_buffer++ = 255;
                    }
                    break;
                }
                while (activ_buf_end > output_buffer) {
                    if (input_buffer < input_buffer_end) {
                        register uint8_t two_points = *input_buffer++; // 4 + 4 bits in input buffer
                        *output_buffer++ = textmode_palette[ two_points & 0x0F ];
                        *output_buffer++ = textmode_palette[ two_points >> 4 ];
                    }
                    else
                        *output_buffer++ = 255;
                }
                break;
            }
            case TEXTMODE_53x30: {
                int y = line >> 1;
                int cur_line = (pos_y << 3) + 7;
                *output_buffer++ = 255;
                for (int x = 0; x < graphics_buffer_width; x++) {
                    if (cur_line == y && pos_x == x) {
                        uint8_t c = textmode_palette[cursor_color & 0xf];
                        for (int bit = 6; bit--;) {
                            *output_buffer++ = c;
                        }
                    } else {
                        const uint16_t offset = (y / 8) * (graphics_buffer_width * 2) + x * 2;
                        const uint8_t c = graphics_buffer[offset];
                        uint8_t glyph_row = pfont_table[c * 8 + y % 8];
                        const uint8_t colorIndex = graphics_buffer[offset + 1];
                        for (int bit = 6; bit--;) {
                            *output_buffer++ = glyph_row & 1
                                                   ? textmode_palette[colorIndex & 0xf] //цвет шрифта
                                                   : textmode_palette[colorIndex >> 4]; //цвет фона
                            glyph_row >>= 1;
                        }
                    }
                }
                *output_buffer = 255;
                break;
            }
#if TEXTMODE_53x60
            case TEXTMODE_53x60: {
                int y = line;
                int cur_line = (pos_y << 3) + 7;
                *output_buffer++ = 255;
                for (int x = 0; x < graphics_buffer_width; x++) {
                    if (cur_line == y && pos_x == x) {
                        uint8_t c = textmode_palette[cursor_color & 0xf];
                        for (int bit = 6; bit--;) {
                            *output_buffer++ = c;
                        }
                    } else {
                        const uint16_t offset = (y / 8) * (graphics_buffer_width * 2) + x * 2;
                        const uint8_t c = graphics_buffer[offset];
                        uint8_t glyph_row = font_table[c * 8 + y % 8];
                        const uint8_t colorIndex = graphics_buffer[offset + 1];
                        for (int bit = 6; bit--;) {
                            *output_buffer++ = glyph_row & 1
                                                   ? textmode_palette[colorIndex & 0xf] //цвет шрифта
                                                   : textmode_palette[colorIndex >> 4]; //цвет фона
                            glyph_row >>= 1;
                        }
                    }
                }
                *output_buffer = 255;
                break;
            }
#endif
            case TEXTMODE_80x30: { // TODO: some other flow
                int y = line / 2;
                int cur_line = (pos_y << 3) + 7;
                *output_buffer++ = 255;
                for (int x = 0; x < graphics_buffer_width; x++) {
                    if (cur_line == y && pos_x == x) {
                        uint8_t c = textmode_palette[cursor_color & 0xf];
                        for (int bit = 4; bit--;) {
                            *output_buffer++ = c;
                        }
                        continue;
                    }
                    const uint16_t offset = (y / 8) * (graphics_buffer_width * 2) + x * 2;
                    const uint8_t c = graphics_buffer[offset];
                    const uint8_t colorIndex = graphics_buffer[offset + 1];
                    uint8_t glyph_row = pfont_table[c * 8 + y % 8]; // TODO: font_4x8
                    for (int bit = 5; bit--;) {
                        if (bit == 1) continue;
                        *output_buffer++ = glyph_row & 1
                                               ? textmode_palette[colorIndex & 0xf] //цвет шрифта
                                               : textmode_palette[colorIndex >> 4]; //цвет фона
                        glyph_row >>= 1;
                    }
                }
                *output_buffer = 255;
                break;
            }
            default:
                /*for (int i = SCREEN_WIDTH; i--;) {
                    uint8_t i_color = *input_buffer++;
                    i_color = (i_color & 0xf0) == 0xf0 ? 255 : i_color;
                    *output_buffer++ = i_color;
                }*/
                break;
        }

        //ССИ
        //для выравнивания синхры

        // --|_|---|_|---|_|----
        //---|___________|-----
        memset(activ_buf + 48, BASE_HDMI_CTRL_INX, 24);
        memset(activ_buf, BASE_HDMI_CTRL_INX + 1, 48);
        memset(activ_buf + 392, BASE_HDMI_CTRL_INX, 8);

        //без выравнивания
        // --|_|---|_|---|_|----
        //------|___________|----
        //   memset(activ_buf+320,BASE_HDMI_CTRL_INX,8);
        //   memset(activ_buf+328,BASE_HDMI_CTRL_INX+1,48);
        //   memset(activ_buf+376,BASE_HDMI_CTRL_INX,24);
    }
    else {
        if ((line >= 490) && (line < 492)) {
            //кадровый синхроимпульс
            //для выравнивания синхры
            // --|_|---|_|---|_|----
            //---|___________|-----
            memset(activ_buf + 48,BASE_HDMI_CTRL_INX + 2, 352);
            memset(activ_buf,BASE_HDMI_CTRL_INX + 3, 48);
            //без выравнивания
            // --|_|---|_|---|_|----
            //-------|___________|----

            // memset(activ_buf,BASE_HDMI_CTRL_INX+2,328);
            // memset(activ_buf+328,BASE_HDMI_CTRL_INX+3,48);
            // memset(activ_buf+376,BASE_HDMI_CTRL_INX+2,24);
        }
        else {
            //ССИ без изображения
            //для выравнивания синхры

            memset(activ_buf + 48,BASE_HDMI_CTRL_INX, 352);
            memset(activ_buf,BASE_HDMI_CTRL_INX + 1, 48);

            // memset(activ_buf,BASE_HDMI_CTRL_INX,328);
            // memset(activ_buf+328,BASE_HDMI_CTRL_INX+1,48);
            // memset(activ_buf+376,BASE_HDMI_CTRL_INX,24);
        };
    }


    // y=(y==524)?0:(y+1);
    // inx_buf_dma++;
}


static inline void irq_remove_handler_DMA_core1() {
    irq_set_enabled(VIDEO_DMA_IRQ, false);
    irq_remove_handler(VIDEO_DMA_IRQ, irq_get_exclusive_handler(VIDEO_DMA_IRQ));
}

static inline void irq_set_exclusive_handler_DMA_core1() {
    irq_set_exclusive_handler(VIDEO_DMA_IRQ, dma_handler_HDMI);
    irq_set_priority(VIDEO_DMA_IRQ, 0);
    irq_set_enabled(VIDEO_DMA_IRQ, true);
}

//деинициализация - инициализация ресурсов
static inline bool inner_init() {
    //выключение прерывания DMA
    if (VIDEO_DMA_IRQ == DMA_IRQ_0) {
        dma_channel_set_irq0_enabled(dma_chan_ctrl, false);
    }
    else {
        dma_channel_set_irq1_enabled(dma_chan_ctrl, false);
    }

    irq_remove_handler_DMA_core1();


    //остановка всех каналов DMA
    dma_hw->abort = (1 << dma_chan_ctrl) | (1 << dma_chan) | (1 << dma_chan_pal_conv) | (
                        1 << dma_chan_pal_conv_ctrl);
    while (dma_hw->abort) tight_loop_contents();

    //выключение SM основной и конвертора

    //pio_sm_restart(PIO_VIDEO, SM_video);
    pio_sm_set_enabled(PIO_VIDEO, SM_video, false);

    //pio_sm_restart(PIO_VIDEO_ADDR, SM_conv);
    pio_sm_set_enabled(PIO_VIDEO_ADDR, SM_conv, false);


    //удаление программ из соответствующих PIO
    pio_remove_program(PIO_VIDEO_ADDR, &pio_program_conv_addr_HDMI, offs_prg1);
    pio_remove_program(PIO_VIDEO, &program_PIO_HDMI, offs_prg0);


    offs_prg1 = pio_add_program(PIO_VIDEO_ADDR, &pio_program_conv_addr_HDMI);
    offs_prg0 = pio_add_program(PIO_VIDEO, &program_PIO_HDMI);
    pio_set_x(PIO_VIDEO_ADDR, SM_conv, ((uint32_t)hdmi_conv_color >> 12));

    //заполнение палитры
    for (int ci = 0; ci < 240; ci++) hdmi_set_palette(ci, palette[ci]); //

    //255 - цвет фона
    hdmi_set_palette(255, palette[255]);


    //240-243 служебные данные(синхра) напрямую вносим в массив -конвертер
    uint64_t* conv_color64 = (uint64_t *)hdmi_conv_color;
    const uint16_t b0 = 0b1101010100;
    const uint16_t b1 = 0b0010101011;
    const uint16_t b2 = 0b0101010100;
    const uint16_t b3 = 0b1010101011;
    const int base_inx = BASE_HDMI_CTRL_INX;

    conv_color64[2 * base_inx + 0] = get_ser_diff_data(b0, b0, b3);
    conv_color64[2 * base_inx + 1] = get_ser_diff_data(b0, b0, b3);

    conv_color64[2 * (base_inx + 1) + 0] = get_ser_diff_data(b0, b0, b2);
    conv_color64[2 * (base_inx + 1) + 1] = get_ser_diff_data(b0, b0, b2);

    conv_color64[2 * (base_inx + 2) + 0] = get_ser_diff_data(b0, b0, b1);
    conv_color64[2 * (base_inx + 2) + 1] = get_ser_diff_data(b0, b0, b1);

    conv_color64[2 * (base_inx + 3) + 0] = get_ser_diff_data(b0, b0, b0);
    conv_color64[2 * (base_inx + 3) + 1] = get_ser_diff_data(b0, b0, b0);

    //настройка PIO SM для конвертации

    pio_sm_config c_c = pio_get_default_sm_config();
    sm_config_set_wrap(&c_c, offs_prg1, offs_prg1 + (pio_program_conv_addr_HDMI.length - 1));
    sm_config_set_in_shift(&c_c, true, false, 32);

    pio_sm_init(PIO_VIDEO_ADDR, SM_conv, offs_prg1, &c_c);
    pio_sm_set_enabled(PIO_VIDEO_ADDR, SM_conv, true);

    //настройка PIO SM для вывода данных
    c_c = pio_get_default_sm_config();
    sm_config_set_wrap(&c_c, offs_prg0, offs_prg0 + (program_PIO_HDMI.length - 1));

    //настройка side set
    sm_config_set_sideset_pins(&c_c,beginHDMI_PIN_clk);
    sm_config_set_sideset(&c_c, 2,false,false);
    for (int i = 0; i < 2; i++) {
        pio_gpio_init(PIO_VIDEO, beginHDMI_PIN_clk + i);
        gpio_set_drive_strength(beginHDMI_PIN_clk + i, GPIO_DRIVE_STRENGTH_12MA);
        gpio_set_slew_rate(beginHDMI_PIN_clk + i, GPIO_SLEW_RATE_FAST);
    }

    pio_sm_set_pins_with_mask(PIO_VIDEO, SM_video, 3u << beginHDMI_PIN_clk, 3u << beginHDMI_PIN_clk);
    pio_sm_set_pindirs_with_mask(PIO_VIDEO, SM_video, 3u << beginHDMI_PIN_clk, 3u << beginHDMI_PIN_clk);
    //пины

    for (int i = 0; i < 6; i++) {
        gpio_set_slew_rate(beginHDMI_PIN_data + i, GPIO_SLEW_RATE_FAST);
        pio_gpio_init(PIO_VIDEO, beginHDMI_PIN_data + i);
        gpio_set_drive_strength(beginHDMI_PIN_data + i, GPIO_DRIVE_STRENGTH_12MA);
        gpio_set_slew_rate(beginHDMI_PIN_data + i, GPIO_SLEW_RATE_FAST);
    }
    pio_sm_set_consecutive_pindirs(PIO_VIDEO, SM_video, beginHDMI_PIN_data, 6, true);
    //конфигурация пинов на выход
    sm_config_set_out_pins(&c_c, beginHDMI_PIN_data, 6);

    //
    sm_config_set_out_shift(&c_c, true, true, 30);
    sm_config_set_fifo_join(&c_c, PIO_FIFO_JOIN_TX);

    sm_config_set_clkdiv(&c_c, clock_get_hz(clk_sys) / 252000000.0f);
    pio_sm_init(PIO_VIDEO, SM_video, offs_prg0, &c_c);
    pio_sm_set_enabled(PIO_VIDEO, SM_video, true);

    //настройки DMA
    dma_lines[0] = &hdmi_conv_color[1024];
    dma_lines[1] = &hdmi_conv_color[1124];

    //основной рабочий канал
    dma_channel_config cfg_dma = dma_channel_get_default_config(dma_chan);
    channel_config_set_transfer_data_size(&cfg_dma, DMA_SIZE_8);
    channel_config_set_chain_to(&cfg_dma, dma_chan_ctrl); // chain to other channel

    channel_config_set_read_increment(&cfg_dma, true);
    channel_config_set_write_increment(&cfg_dma, false);


    uint dreq = DREQ_PIO1_TX0 + SM_conv;
    if (PIO_VIDEO_ADDR == pio0) dreq = DREQ_PIO0_TX0 + SM_conv;

    channel_config_set_dreq(&cfg_dma, dreq);

    dma_channel_configure(
        dma_chan,
        &cfg_dma,
        &PIO_VIDEO_ADDR->txf[SM_conv], // Write address
        &dma_lines[0][0], // read address
        400, //
        false // Don't start yet
    );

    //контрольный канал для основного
    cfg_dma = dma_channel_get_default_config(dma_chan_ctrl);
    channel_config_set_transfer_data_size(&cfg_dma, DMA_SIZE_32);
    channel_config_set_chain_to(&cfg_dma, dma_chan); // chain to other channel

    channel_config_set_read_increment(&cfg_dma, false);
    channel_config_set_write_increment(&cfg_dma, false);

    DMA_BUF_ADDR[0] = &dma_lines[0][0];
    DMA_BUF_ADDR[1] = &dma_lines[1][0];

    dma_channel_configure(
        dma_chan_ctrl,
        &cfg_dma,
        &dma_hw->ch[dma_chan].read_addr, // Write address
        &DMA_BUF_ADDR[0], // read address
        1, //
        false // Don't start yet
    );

    //канал - конвертер палитры

    cfg_dma = dma_channel_get_default_config(dma_chan_pal_conv);
    channel_config_set_transfer_data_size(&cfg_dma, DMA_SIZE_32);
    channel_config_set_chain_to(&cfg_dma, dma_chan_pal_conv_ctrl); // chain to other channel

    channel_config_set_read_increment(&cfg_dma, true);
    channel_config_set_write_increment(&cfg_dma, false);

    dreq = DREQ_PIO1_TX0 + SM_video;
    if (PIO_VIDEO == pio0) dreq = DREQ_PIO0_TX0 + SM_video;

    channel_config_set_dreq(&cfg_dma, dreq);

    dma_channel_configure(
        dma_chan_pal_conv,
        &cfg_dma,
        &PIO_VIDEO->txf[SM_video], // Write address
        &hdmi_conv_color[0], // read address
        4, //
        false // Don't start yet
    );

    //канал управления конвертером палитры

    cfg_dma = dma_channel_get_default_config(dma_chan_pal_conv_ctrl);
    channel_config_set_transfer_data_size(&cfg_dma, DMA_SIZE_32);
    channel_config_set_chain_to(&cfg_dma, dma_chan_pal_conv); // chain to other channel

    channel_config_set_read_increment(&cfg_dma, false);
    channel_config_set_write_increment(&cfg_dma, false);

    dreq = DREQ_PIO1_RX0 + SM_conv;
    if (PIO_VIDEO_ADDR == pio0) dreq = DREQ_PIO0_RX0 + SM_conv;

    channel_config_set_dreq(&cfg_dma, dreq);

    dma_channel_configure(
        dma_chan_pal_conv_ctrl,
        &cfg_dma,
        &dma_hw->ch[dma_chan_pal_conv].read_addr, // Write address
        &PIO_VIDEO_ADDR->rxf[SM_conv], // read address
        1, //
        true // start yet
    );

    //стартуем прерывание и канал
    if (VIDEO_DMA_IRQ == DMA_IRQ_0) {
        dma_channel_acknowledge_irq0(dma_chan_ctrl);
        dma_channel_set_irq0_enabled(dma_chan_ctrl, true);
    }
    else {
        dma_channel_acknowledge_irq1(dma_chan_ctrl);
        dma_channel_set_irq1_enabled(dma_chan_ctrl, true);
    }

    irq_set_exclusive_handler_DMA_core1();

    dma_start_channel_mask((1u << dma_chan_ctrl));

    return true;
};

#include "font6x8.h"

//выбор видеорежима
void hdmi_set_mode(int mode) {
    font_width = 6;
    font_height = 8;
    font_table = font_6x8;
    switch (mode) {
        case TEXTMODE_53x30:
            graphics_buffer_width = 53;
            graphics_buffer_height = 30;
            bitness = 16;
            break;
#if TEXTMODE_53x60
        case TEXTMODE_53x60:
            graphics_buffer_width = 53;
            graphics_buffer_height = 60;
            bitness = 16;
            break;
#endif
        case TEXTMODE_80x30:
            graphics_buffer_width = 80;
            graphics_buffer_height = 30;
            bitness = 16;
            break;
        case VGA_320x240x16:
            graphics_buffer_width = 320;
            graphics_buffer_height = 240;
            bitness = 4;
            break;
        default: // override unexpected modes by 0 (TEXTMODE_53x30)
            mode = TEXTMODE_53x30;
            graphics_buffer_width = 53;
            graphics_buffer_height = 30;
            bitness = 16;
            break;
    }
    graphics_mode = mode;
    pos_x = 0;
    pos_y = 0;
    if (!lock_buffer) {
        if (graphics_buffer) vPortFree(graphics_buffer);
        graphics_buffer = pvPortCalloc((graphics_buffer_width * graphics_buffer_height * bitness) >> 5, 4);
    }
};

void hdmi_set_palette(uint8_t i, uint32_t color888) {
    palette[i] = color888 & 0x00ffffff;
    if ((i >= BASE_HDMI_CTRL_INX) && (i != 255))
        return; //не записываем "служебные" цвета
    uint64_t* conv_color64 = (uint64_t *)hdmi_conv_color;
    const uint8_t R = (color888 >> 16) & 0xff;
    const uint8_t G = (color888 >> 8) & 0xff;
    const uint8_t B = (color888 >> 0) & 0xff;
    conv_color64[i * 2] = get_ser_diff_data(tmds_encoder(R), tmds_encoder(G), tmds_encoder(B));
    conv_color64[i * 2 + 1] = conv_color64[i * 2] ^ 0x0003ffffffffffffl;
};

//выделение и настройка общих ресурсов - 4 DMA канала, PIO программ и 2 SM
void hdmi_init() {
    //настройка PIO
    SM_video = pio_claim_unused_sm(PIO_VIDEO, true);
    SM_conv = pio_claim_unused_sm(PIO_VIDEO_ADDR, true);
    //выделение и преднастройка DMA каналов
    dma_chan_ctrl = dma_claim_unused_channel(true);
    dma_chan = dma_claim_unused_channel(true);
    dma_chan_pal_conv_ctrl = dma_claim_unused_channel(true);
    dma_chan_pal_conv = dma_claim_unused_channel(true);


    // FIXME сделать конфигурацию пользователем
    hdmi_set_palette(200, RGB888(0x00, 0x00, 0x00)); //black
    hdmi_set_palette(201, RGB888(0x00, 0x00, 0xC4)); //blue
    hdmi_set_palette(202, RGB888(0x00, 0xC4, 0x00)); //green
    hdmi_set_palette(203, RGB888(0x00, 0xC4, 0xC4)); //cyan
    hdmi_set_palette(204, RGB888(0xC4, 0x00, 0x00)); //red
    hdmi_set_palette(205, RGB888(0xC4, 0x00, 0xC4)); //magenta
    hdmi_set_palette(206, RGB888(0xC4, 0x7E, 0x00)); //brown
    hdmi_set_palette(207, RGB888(0xC4, 0xC4, 0xC4)); //light gray
    hdmi_set_palette(208, RGB888(0x4E, 0x4E, 0x4E)); //dark gray
    hdmi_set_palette(209, RGB888(0x4E, 0x4E, 0xDC)); //light blue
    hdmi_set_palette(210, RGB888(0x4E, 0xDC, 0x4E)); //light green
    hdmi_set_palette(211, RGB888(0x4E, 0xF3, 0xF3)); //light cyan
    hdmi_set_palette(212, RGB888(0xDC, 0x4E, 0x4E)); //light red
    hdmi_set_palette(213, RGB888(0xF3, 0x4E, 0xF3)); //light magenta
    hdmi_set_palette(214, RGB888(0xF3, 0xF3, 0x4E)); //yellow
    hdmi_set_palette(215, RGB888(0xFF, 0xFF, 0xFF)); //white

    inner_init();
}

void hdmi_set_bgcolor(uint32_t color888) //определяем зарезервированный цвет в палитре
{
    hdmi_set_palette(255, color888);
};

void hdmi_set_offset(int x, int y) {
    graphics_buffer_shift_x = x;
    graphics_buffer_shift_y = y;
};


void hdmi_clr_scr(const uint8_t color) {
    if (graphics_buffer) {
        memset(graphics_buffer, color, (graphics_buffer_height * graphics_buffer_width * bitness) >> 3);
    }
}

void hdmi_driver_init(void) {
    palette = pvPortCalloc(256, sizeof(uint32_t));
 // TODO:   set_vga_dma_handler_impl(dma_handler_VGA_impl);
    hdmi_set_bgcolor(0x000000);
  // ??  init_palette();
}

void hdmi_cleanup(void) {
    /* TODO:
    vga_context_t* cleanup = vga_context;
    vga_context = 0;
    if (cleanup) {
        if (!lock_buffer && cleanup->graphics_buffer) vPortFree(cleanup->graphics_buffer);
        if (cleanup->lines_pattern_data) vPortFree(cleanup->lines_pattern_data);
        if (cleanup->txt_palette_fast) vPortFree(cleanup->txt_palette_fast);
        vPortFree(cleanup);
    }
    */
}

bool hdmi_is_mode_text(int mode) {
    return mode <= TEXTMODE_80x30;
}

bool hdmi_is_text_mode() {
    return hdmi_is_mode_text(graphics_mode);
}

int hdmi_get_mode(void) {
    return graphics_mode;
}

uint32_t hdmi_console_width(void) {
    if (graphics_mode == VGA_320x240x16) {
        return graphics_buffer_width / font_width;
    }
    return graphics_buffer_width;
}
uint32_t hdmi_console_height(void) {
    if (graphics_mode == VGA_320x240x16) {
        return graphics_buffer_height / font_height;
    }
    return graphics_buffer_height;
}
uint32_t hdmi_screen_width(void) {
    return graphics_buffer_width;
}
uint32_t hdmi_screen_height(void) {
    return graphics_buffer_height;
}
uint8_t* get_hdmi_buffer(void) {
    return graphics_buffer;
}
void set_hdmi_buffer(uint8_t* b) {
    graphics_buffer = b;
}

uint8_t get_hdmi_buffer_bitness(void) {
    return bitness;
}

size_t hdmi_buffer_size() {
    if (!graphics_buffer) return 0;
    return (graphics_buffer_height * graphics_buffer_width * bitness) >> 3;
}

void hdmi_lock_buffer(bool b) {
    lock_buffer = b;
}

void hdmi_set_cursor_color(uint8_t color) {
    _cursor_color = color;
}
