#include "vga.h"
#include <hardware/dma.h>
#include <hardware/irq.h>
#include <hardware/pio.h>
#include <hardware/clocks.h>


//  ----

#define PIO_VGA (pio0)
#define beginVGA_PIN (6)
#define VGA_DMA_IRQ (DMA_IRQ_0)

static int _SM_VGA = -1;
static int dma_chan_ctrl;
static int dma_chan;

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

static volatile dma_handler_impl_fn pdma_handler_VGA_impl = NULL;

void set_vga_dma_handler_impl(dma_handler_impl_fn impl) {
    pdma_handler_VGA_impl = impl;
}

void __time_critical_func(dma_handler_VGA)() {
    dma_hw->ints0 = 1u << dma_chan_ctrl;
    uint8_t* data = pdma_handler_VGA_impl();
    dma_channel_set_read_addr(dma_chan_ctrl, data, false);
}

void set_vga_clkdiv(uint32_t pixel_clock, uint32_t line_size) {
    double fdiv = clock_get_hz(clk_sys) / (pixel_clock * 1.0); // частота пиксельклока
    uint32_t div32 = (uint32_t)(fdiv * (1 << 16) + 0.0);
    PIO_VGA->sm[_SM_VGA].clkdiv = div32 & 0xfffff000; //делитель для конкретной sm
    dma_channel_set_trans_count(dma_chan, line_size >> 2, false);
}

void vga_init() {
    static uint32_t* initial_lines_pattern[4]; // TODO: remove W/A
    if (_SM_VGA != -1) return; // do not init it twice
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
        &initial_lines_pattern[0], // read address
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
        &initial_lines_pattern[0], // read address
        1, //
        false // Don't start yet
    );
    irq_set_exclusive_handler(VGA_DMA_IRQ, dma_handler_VGA);
    dma_channel_set_irq0_enabled(dma_chan_ctrl, true);
    irq_set_enabled(VGA_DMA_IRQ, true);
    dma_start_channel_mask((1u << dma_chan));
};

void vga_dma_channel_set_read_addr(const volatile void* addr) {
    dma_channel_set_read_addr(dma_chan, addr, false);
}
