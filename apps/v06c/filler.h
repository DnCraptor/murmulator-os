#include "memory.h"
#include "vio.h"
#include "8253.h"

#define DEFAULT_BORDER_WIDTH 10
#define DEFAULT_SCREEN_WIDTH (512 + 2*(DEFAULT_BORDER_WIDTH))
#define DEFAULT_SCREEN_HEIGHT (256 + 16 + 16)
#define DEFAULT_CENTER_OFFSET (152 - DEFAULT_BORDER_WIDTH)

enum ResetMode
{
    BLKSBR,
    BLKVVOD,
    LOADROM
};

namespace filler {
    constexpr int center_offset = DEFAULT_CENTER_OFFSET;
    constexpr int screen_width = DEFAULT_SCREEN_WIDTH;
    constexpr int first_visible_line = 24; // typical v06x: 24
    constexpr int first_raster_line = 40;
    constexpr int last_raster_line = 40 + 256;
    constexpr int last_visible_line = 311;

    extern int raster_line;
    extern int irq;
    extern int inte;
    extern int write_buffer;
    extern int ay_bufpos_reg;

    extern volatile int v06x_framecount;
    extern volatile int v06x_frame_cycles;

    typedef void (*onosd_t)(void);
    extern onosd_t onosd;

    void init(Memory * memory, IO * _io, uint8_t * buf1, uint8_t * buf2, I8253 * vi53, WavPlayer * tape_player);
    void frame_start();
    int fill(int ncycles, int commit_time, int commit_time_pal);
    int fill_noout(int ncycles);
    int fill_void(int ncycles, int commit_time, int commit_time_pal);
    int fill_void_noout(int ncycles);
    int fake_fill(int ncycles, int commit_time, int commit_time_pal);
    void write_pal(uint8_t adr8, uint8_t rgb);

    int bob(int maxframes);
    void reset(ResetMode mode);
}
