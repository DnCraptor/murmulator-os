#include "m-os-api.h"
#include "m-os-api-sdtfn.h"
#include "m-os-api-timer.h"
#include "m-os-api-math.h"
/// TODO:
#undef switch

#include "timing.h"
#include "assembly.h"
#include "coder.h"
#include "mp3common.h"
#include "mp3dec.h"
///#include "mpadecobjfixpt.h"
#include "statname.h"
#include "wave.h"
#include "music_file.h"

#define WORKING_SIZE        16000
#define RAM_BUFFER_LENGTH   6000

int16_t d_buff[RAM_BUFFER_LENGTH];
unsigned char working[WORKING_SIZE];

int main(void) {
    marked_to_exit = false;
    cmd_ctx_t* ctx = get_cmd_ctx();
    if (ctx->argc < 2 || ctx->argc > 3) {
        fprintf(ctx->std_err, "Usage: mp3wav [file.mp3] [file.wav]\n");
        return 1;
    }
    char buf[128];
    char* source = ctx->argv[1];
    char* target = buf;
    if (ctx->argc == 3) {
        target = ctx->argv[2];
    } else {
        snprintf(target, 128, "%s.wav");
    }

    printf("In: %s, Out: %s\n", source, target);

    InitTimer();

    music_file mf;
    FIL outfile;
    // Open the file, pass in the buffer used to store source data 
    // read from file
    if (!musicFileCreate(&mf, source, working, WORKING_SIZE)) 
    {
        printf("mp3FileCreate error\n");
        return -1;

    }
    
    // Create the file to hold the PCM output
    if (f_open(&outfile, target, FA_CREATE_ALWAYS | FA_WRITE) != FR_OK)
    {
        printf("Outfile open error\n");
        return -1;
    }

    // Get the input file sampling rate
    uint32_t sample_rate = musicFileGetSampleRate(&mf);
    uint16_t num_channels =  musicFileGetChannels(&mf);

    printf("Sample rate: %u Channels: %u\n", sample_rate, num_channels);

    return 0;
}

#include "timing.c"
#include "bitstream.c"
#include "buffers.c"
#include "dct32.c"
#include "dequant.c"
#include "dqchan.c"
#include "huffman.c"
#include "hufftabs.c"
#include "imdct.c"
#include "mp3dec.c"
#include "mp3tabs.c"
#include "polyphase.c"
#include "scalfact.c"
#include "stproc.c"
#include "subband.c"
#include "trigtabs.c"
#include "wave.c"
#include "music_file.c"
