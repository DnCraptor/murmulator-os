/***
 * Вектор-06Ц эмулятор-порт под Murmulator OS
 * original code base: https://github.com/svofski/v06x-tiny
 */

extern "C" {
#include "m-os-api.h"
#include "m-os-api-sdtfn.h"
//#include "m-os-api-timer.h"
#include "m-os-api-math.h"
}
// TODO:
#undef switch

#define IRAM_ATTR
#define TOTAL_MEMORY (64 * 1024 + 256 * 1024)
#define VI53_CLOCKS_PER_SAMPLE  48  // 1.5MHz clocks per output sample
#define AUDIO_SCALE_8253        8   // shift 8253 by this many bits
#define AUDIO_SAMPLES_PER_FRAME (312*2)
#define AUDIO_SAMPLERATE        (AUDIO_SAMPLES_PER_FRAME * 50)
#define AUDIO_SAMPLE_SIZE       2 // sizeof(int16_t)

#include "options.h"
#include "memory.h"
#include "fd1793.h"
#include "wav.h"
#include "8253.h"
#include "AySound.h"
#include "vio.h"
#include "filler.h"

static Memory * memory;
static Wav * wav;
static IO * io;

int main(void) {
    cmd_ctx_t* ctx = get_cmd_ctx();
    marked_to_exit = false;

    Options.nosound = true;
    Options.nofilter = true;
    Options.log.fdc = false;

    memory = new Memory(); // it gets allocated in PSRAM (TODO: swap support)
    FD1793* fdc = new FD1793();
    wav = new Wav();
    WavPlayer* tape_player = new WavPlayer(*wav);
    I8253* timer = new I8253();

    // AY Sound
    AySound::init();
    AySound::set_sound_format(AUDIO_SAMPLERATE, 1, 8);
    AySound::set_stereo(AYEMU_MONO, NULL);
    AySound::reset();

    io = new IO(*memory, *timer, *fdc, *tape_player);
    filler::init(memory, io, get_buffer(), NULL, timer, tape_player);

//TODO:
    delete io;
    delete timer;
    delete tape_player;
    delete wav;
    delete fdc;
    delete memory;
    return 0;
}

#include "options.cpp"
#include "memory.cpp"
#include "8253.cpp"
#include "AySound.cpp"
#include "filler.cpp"
