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

static int16_t d_buff[RAM_BUFFER_LENGTH];
static unsigned char working[WORKING_SIZE];
static char buf[128];

int main(void) {
    marked_to_exit = false;
    cmd_ctx_t* ctx = get_cmd_ctx();
    if (ctx->argc < 2 || ctx->argc > 3) {
        fprintf(ctx->std_err, "Usage: mp3wav [file.mp3] [file.wav]\n");
        return 1;
    }
    char* source = ctx->argv[1];
    char* target = buf;
    if (ctx->argc == 3) {
        target = ctx->argv[2];
    } else {
        snprintf(target, 128, "%s.wav", source);
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

    // Write the wav header to the output file
    writeWavHeader(&outfile, sample_rate, num_channels);

    // loop through all of the data
    bool success = true;
    uint32_t num_samples = 0;
    UINT written = 0;
    uint32_t read = 0;

    int32_t start_time; 
    int32_t end_time;
    int32_t diff_time;
    int32_t total_dec_time = 0;

    // If looping stop after 5 secs (off target) or 30 secs (On target)
    int32_t stop_time = 
#ifdef NO_LOOP
                        999999;
#else
#ifdef OFF_TARGET
                        5;
#else
                        30;
#endif
#endif

    printf("Read start\n");

    do
    {
        // Receive the blocks
        start_time = ReadTimer();

        success = musicFileRead(&mf, d_buff, RAM_BUFFER_LENGTH, &read);
        end_time = ReadTimer();
        diff_time = CalcTimeDifference(start_time, end_time);
        total_dec_time += diff_time;

        // Write any data received
        if (success && read)
        {
            num_samples += read / num_channels;

            if ((f_write(&outfile, d_buff, read * sizeof(int16_t), &written) != FR_OK) || (written != read * sizeof(int16_t)))
            {
                printf("Error in f_write\n");
                success = false;
            }
        }
        else
        {
            if (!success)
            {
                printf("Error in mp3FileRead\n");
            }
            else
            {
                printf("Read complete\n");
            }
        }
    } while (success && read &&(total_dec_time / GetClockFrequency() < stop_time));
    
    if (success)
    {
        // update the file header of the output file
        updateWavHeader(&outfile, num_samples, num_channels);
    }

    // Report the performance
    float time_to_decode = (float)total_dec_time / GetClockFrequency();
    float time_decoded = (float)num_samples / mf.sample_rate;
    printf("Time to decode = %6.2f s. Decoded Length = %6.2f s. Ratio = %6.2f\n\n", 
           time_to_decode,
           time_decoded,
           time_decoded / time_to_decode) ;

    // Close the files
    f_close(&outfile);
    musicFileClose(&mf);

    return 0;
}

#define MPDEC_ALLOCATOR(x) malloc(x)
#define SAFE_FREE(x) free(x)

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
