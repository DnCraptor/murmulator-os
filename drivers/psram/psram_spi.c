#include "psram_spi.h"

static psram_spi_inst_t psram_spi;

#define ITE_PSRAM (1ul << 20)
#define MAX_PSRAM (512ul << 20)

uint32_t psram_size() {
    uint32_t res = 0;
    for (res = ITE_PSRAM; res < MAX_PSRAM; res += ITE_PSRAM) {
        psram_write32(&psram_spi, res, res);
        if (res != psram_read32(&psram_spi, res)) {
            res -= ITE_PSRAM;
            return res;
        }
    }
    return res - psram_read32(&psram_spi, ITE_PSRAM) + ITE_PSRAM;
}

uint32_t init_psram() {
    psram_spi = psram_spi_init_clkdiv(pio0, -1, 2.0, true);
    return psram_size();
}

void psram_cleanup() {
    //logMsg("PSRAM cleanup"); // TODO: block mode, ensure diapason
    for (uint32_t addr32 = (1ul << 20); addr32 < (2ul << 20); addr32 += 4) {
        psram_write32(&psram_spi, addr32, 0);
    }
}

void write8psram(uint32_t addr32, uint8_t v) {
    psram_write8(&psram_spi, addr32, v);
}

void write16psram(uint32_t addr32, uint16_t v) {
    psram_write16(&psram_spi, addr32, v);
}

void write32psram(uint32_t addr32, uint32_t v) {
    psram_write32(&psram_spi, addr32, v);
}

uint8_t read8psram(uint32_t addr32) {
    return psram_read8(&psram_spi, addr32);
}

uint16_t read16psram(uint32_t addr32) {
    return psram_read16(&psram_spi, addr32);
}

uint32_t read32psram(uint32_t addr32) {
    return psram_read32(&psram_spi, addr32);
}

#include <stdio.h>

#if defined(PSRAM_ASYNC) && defined(PSRAM_ASYNC_SYNCHRONIZE)
void __isr psram_dma_complete_handler() {
#if PSRAM_ASYNC_DMA_IRQ == 0
    dma_hw->ints0 = 1u << async_spi_inst->async_dma_chan;
#elif PSRAM_ASYNC_DMA_IRQ == 1
    dma_hw->ints1 = 1u << async_spi_inst->async_dma_chan;
#else
#error "PSRAM_ASYNC defined without PSRAM_ASYNC_DMA_IRQ set to 0 or 1"
#endif
    /* putchar('@'); */
#if defined(PSRAM_MUTEX)
    mutex_exit(&async_spi_inst->mtx);
#elif defined(PSRAM_SPINLOCK)
    spin_unlock(async_spi_inst->spinlock, async_spi_inst->spin_irq_state);
#endif
}
#endif // defined(PSRAM_ASYNC) && defined(PSRAM_ASYNC_SYNCHRONIZE)

psram_spi_inst_t psram_spi_init_clkdiv(PIO pio, int sm, float clkdiv, bool fudge) {
    psram_spi_inst_t spi;
    spi.pio = pio;
    spi.offset = pio_add_program(spi.pio, fudge ? &spi_psram_fudge_program : &spi_psram_program);
    if (sm == -1) {
        spi.sm = pio_claim_unused_sm(spi.pio, true);
    } else {
        spi.sm = sm;
    }
#if defined(PSRAM_MUTEX)
    mutex_init(&spi.mtx);
#elif defined(PSRAM_SPINLOCK)
    int spin_id = spin_lock_claim_unused(true);
    spi.spinlock = spin_lock_init(spin_id);
#endif

    gpio_set_drive_strength(PSRAM_PIN_CS, GPIO_DRIVE_STRENGTH_4MA);
    gpio_set_drive_strength(PSRAM_PIN_SCK, GPIO_DRIVE_STRENGTH_4MA);
    gpio_set_drive_strength(PSRAM_PIN_MOSI, GPIO_DRIVE_STRENGTH_4MA);
    /* gpio_set_slew_rate(PSRAM_PIN_CS, GPIO_SLEW_RATE_FAST); */
    /* gpio_set_slew_rate(PSRAM_PIN_SCK, GPIO_SLEW_RATE_FAST); */
    /* gpio_set_slew_rate(PSRAM_PIN_MOSI, GPIO_SLEW_RATE_FAST); */

    pio_spi_psram_cs_init(spi.pio, spi.sm, spi.offset, 8 /*n_bits*/, clkdiv, fudge, PSRAM_PIN_CS, PSRAM_PIN_MOSI, PSRAM_PIN_MISO);

    // Write DMA channel setup
    spi.write_dma_chan = dma_claim_unused_channel(true);
    spi.write_dma_chan_config = dma_channel_get_default_config(spi.write_dma_chan);
    channel_config_set_transfer_data_size(&spi.write_dma_chan_config, DMA_SIZE_8);
    channel_config_set_read_increment(&spi.write_dma_chan_config, true);
    channel_config_set_write_increment(&spi.write_dma_chan_config, false);
    channel_config_set_dreq(&spi.write_dma_chan_config, pio_get_dreq(spi.pio, spi.sm, true));
    dma_channel_set_write_addr(spi.write_dma_chan, &spi.pio->txf[spi.sm], false);
    dma_channel_set_config(spi.write_dma_chan, &spi.write_dma_chan_config, false);

    // Read DMA channel setup
    spi.read_dma_chan = dma_claim_unused_channel(true);
    spi.read_dma_chan_config = dma_channel_get_default_config(spi.read_dma_chan);
    channel_config_set_transfer_data_size(&spi.read_dma_chan_config, DMA_SIZE_8);
    channel_config_set_read_increment(&spi.read_dma_chan_config, false);
    channel_config_set_write_increment(&spi.read_dma_chan_config, true);
    channel_config_set_dreq(&spi.read_dma_chan_config, pio_get_dreq(spi.pio, spi.sm, false));
    dma_channel_set_read_addr(spi.read_dma_chan, &spi.pio->rxf[spi.sm], false);
    dma_channel_set_config(spi.read_dma_chan, &spi.read_dma_chan_config, false);

#if defined(PSRAM_ASYNC)
    // Asynchronous DMA channel setup
    spi.async_dma_chan = dma_claim_unused_channel(true);
    spi.async_dma_chan_config = dma_channel_get_default_config(spi.async_dma_chan);
    channel_config_set_transfer_data_size(&spi.async_dma_chan_config, DMA_SIZE_8);
    channel_config_set_read_increment(&spi.async_dma_chan_config, true);
    channel_config_set_write_increment(&spi.async_dma_chan_config, false);
    channel_config_set_dreq(&spi.async_dma_chan_config, pio_get_dreq(spi.pio, spi.sm, true));
    dma_channel_set_write_addr(spi.async_dma_chan, &spi.pio->txf[spi.sm], false);
    dma_channel_set_config(spi.async_dma_chan, &spi.async_dma_chan_config, false);

#if defined(PSRAM_ASYNC_COMPLETE)
    irq_set_exclusive_handler(DMA_IRQ_0 + PSRAM_ASYNC_DMA_IRQ, psram_dma_complete_handler);
    dma_irqn_set_channel_enabled(PSRAM_ASYNC_DMA_IRQ, spi.async_dma_chan, true);
    irq_set_enabled(DMA_IRQ_0 + PSRAM_ASYNC_DMA_IRQ, true);
#endif // defined(PSRAM_ASYNC_COMPLETE)
#endif // defined(PSRAM_ASYNC)

    uint8_t psram_reset_en_cmd[] = {
        8,      // 8 bits to write
        0,      // 0 bits to read
        0x66u   // Reset enable command
    };
    pio_spi_write_read_dma_blocking(&spi, psram_reset_en_cmd, 3, 0, 0);
    busy_wait_us(50);
    uint8_t psram_reset_cmd[] = {
        8,      // 8 bits to write
        0,      // 0 bits to read
        0x99u   // Reset command
    };
    pio_spi_write_read_dma_blocking(&spi, psram_reset_cmd, 3, 0, 0);
    busy_wait_us(100);
    
    return spi;
};

psram_spi_inst_t psram_spi_init(PIO pio, int sm) {
    return psram_spi_init_clkdiv(pio, sm, 1.8, true);
}

void psram_spi_uninit(psram_spi_inst_t spi, bool fudge) {
#if defined(PSRAM_ASYNC)
    // Asynchronous DMA channel teardown
    dma_channel_unclaim(spi.async_dma_chan);
#if defined(PSRAM_ASYNC_COMPLETE)
    irq_set_enabled(DMA_IRQ_0 + PSRAM_ASYNC_DMA_IRQ, false);
    dma_irqn_set_channel_enabled(PSRAM_ASYNC_DMA_IRQ, spi.async_dma_chan, false);
    irq_remove_handler(DMA_IRQ_0 + PSRAM_ASYNC_DMA_IRQ, psram_dma_complete_handler);
#endif // defined(PSRAM_ASYNC_COMPLETE)
#endif // defined(PSRAM_ASYNC)

    // Write DMA channel teardown
    dma_channel_unclaim(spi.write_dma_chan);

    // Read DMA channel teardown
    dma_channel_unclaim(spi.read_dma_chan);

#if defined(PSRAM_SPINLOCK)
    int spin_id = spin_lock_get_num(spi.spinlock);
    spin_lock_unclaim(spin_id);
#endif

    pio_sm_unclaim(spi.pio, spi.sm);
    pio_remove_program(spi.pio, fudge ? &spi_psram_fudge_program : &spi_psram_program, spi.offset);
}

// TODO:
void psram_jedec_id(uint8_t rx[4]) {
    uint8_t tx[4] = {0x9f};
    pio_spi_write_read_dma_blocking(&psram_spi, tx, 4, rx, 4);
}
