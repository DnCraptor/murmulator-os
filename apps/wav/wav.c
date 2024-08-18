#include "m-os-api.h"
#include "m-os-api-sdtfn.h"

/*
from https://eax.me/scala-wav/
Смещение   Байт  Описание
------------------------------------------------------------------
0x00 (00)  4     "RIFF", сигнатура
0x04 (04)  4     размер фала в байтах минус 8
0x08 (08)  8     "WAVEfmt "
0x10 (16)  4     16 для PCM, оставшийся размер заголовка
0x14 (20)  2     1 для PCM, иначе есть какое-то сжатие
0x16 (22)  2     число каналов - 1, 2, 3...
0x18 (24)  4     частота дискретизации
0x1c (28)  4     байт на одну секунду воспроизведения
0x20 (32)  2     байт для одного сэпла включая все каналы
0x22 (34)  2     бит в сэмпле на один канал
0x24 (36)  4     "data" (id сабчанка)
0x28 (40)  4     сколько байт данных идет далее (размер сабчанка)
0x2c (44)  -     данные
*/

typedef struct wav {
    char RIFF[4];
    uint32_t f_szie; // -8
    char WAVEfmt[8];
    uint32_t h_size; // 16
    uint16_t pcm; // 1
    uint16_t ch; // 1 or 2
    uint32_t freq;
    uint32_t byte_per_second;
    uint16_t byte_per_sample;
    uint16_t bit_per_sample; // for 1 channel
    char data[4]; // id for subchunk
    uint32_t subchunk_size;
} wav_t;

static char* b1;
static char* b2;

static volatile size_t b1_sz;
static volatile size_t b2_sz;

static volatile bool b1_locked;
static volatile bool b2_locked;

static char* pcm_end_callback(size_t* size) {
    if (b1_locked) {
        b2_locked = true;
        b1_locked = false;
        if (b2_sz) {
            //printf("switch to b2\n");
            b2_locked = true;
            *size = b2_sz;
            return b2;
        }
        //printf("no b2 data\n");
        b2_locked = false;
        return NULL;
    }
    if (b2_locked) {
        b1_locked = true;
        b2_locked = false;
        if (b1_sz) {
            //printf("switch to b1\n");
            b1_locked = true;
            *size = b1_sz;
            return b2;
        }
        //printf("no b1 data\n");
        b1_locked = false;
        return NULL;
    }
    //printf("no b1/b2 locked\n");
    return NULL;
}

inline static void wait4end(void) {
    while(b1_locked || b2_locked) {
        vTaskDelay(1);
    }
}

inline static void wait4b1(void) {
    while(b1_locked) {
        vTaskDelay(1);
    }
}

inline static void wait4b2(void) {
    while(b2_locked) {
        vTaskDelay(1);
    }
}

int main(void) {
    cmd_ctx_t* ctx = get_cmd_ctx();
    if (ctx->argc != 2) {
        fprintf(ctx->std_err, "Usage: wav [file]\n");
        return 1;
    }
    int res = 0;
    FIL* f = (FIL*)malloc(sizeof(FIL));
    if (f_open(f, ctx->argv[1], FA_READ) != FR_OK) {
        fprintf(ctx->std_err, "Unable to open file: '%s'\n", ctx->argv[1]);
        res = 2;
        goto e1;
    }
    wav_t* w = (wav_t*)malloc(sizeof(wav_t));
    UINT rb;
    if (f_read(f, w, sizeof(wav_t), &rb) != FR_OK) {
        fprintf(ctx->std_err, "Unable to read file: '%s'\n", ctx->argv[1]);
        res = 3;
        goto e2;
    }
    if (strncmp("RIFF", w->RIFF, 4) != 0) {
        fprintf(ctx->std_err, "Unsupported file type: '%s' (RIFF is expected)\n", ctx->argv[1]);
        res = 4;
        goto e2;
    }
    if (strncmp("WAVEfmt ", w->WAVEfmt, 8) != 0) {
        fprintf(ctx->std_err, "Unsupported file format: '%s' (WAVEfmt is expected)\n", ctx->argv[1]);
        res = 5;
        goto e2;
    }

    printf("       file name: %s\n", ctx->argv[1]);
    printf("       file size: %d\n", w->f_szie + 8);
    printf("     header size: %d\n", w->h_size);
    printf("             pcm: %d\n", w->pcm);
    printf("        channels: %d\n", w->ch);
    printf("            hreq: %d Hz\n", w->freq);
    printf("bytes per second: %d\n", w->byte_per_second);
    printf("bytes per sample: %d\n", w->byte_per_sample);
    printf("bits per channel: %d\n --- \n", w->bit_per_sample);

    printf("   data chunk id: %c%c%c%c\n", w->data[0], w->data[1], w->data[2], w->data[3]);
    printf(" data chunk size: %d (%d KB)\n", w->subchunk_size, w->subchunk_size >> 10);

    if (w->pcm != 1) {
        fprintf(ctx->std_err, "Unsupported file format: PCM = %d (1 is expected)\n", w->ch);
        res = 6;
        goto e2;
    }
    if (w->ch != 1 && w->ch != 2) {
        fprintf(ctx->std_err, "Unsupported number of channels: %d (1 or 2 is expected)\n", w->ch);
        res = 7;
        goto e2;
    }
    if (w->h_size != 16) {
        fprintf(ctx->std_err, "Unsupported header size: %d (16 is expected)\n", w->h_size);
        res = 8;
        goto e2;
    }
    HeapStats_t* stat = (HeapStats_t*)malloc(sizeof(HeapStats_t));
    vPortGetHeapStats(stat);
    size_t ONE_BUFF_SIZE = stat->xMinimumEverFreeBytesRemaining >> 2; // using half of free continues block
    free(stat);

    pcm_setup(w->freq);
    b1 = (char*)malloc(ONE_BUFF_SIZE);
    b2 = (char*)malloc(ONE_BUFF_SIZE);
    printf("2 buffers allocated: %d (%dKB) each\n", ONE_BUFF_SIZE, ONE_BUFF_SIZE >> 10);

    b2_sz = 0;
    b2_locked = false;
    //printf("read b1\n");
    if (f_read(f, b1, ONE_BUFF_SIZE, &b1_sz) == FR_OK && b1_sz) {
        b1_locked = true;
        pcm_set_buffer(b1, w->ch, rb, pcm_end_callback);
    }
    f_read(f, b2, ONE_BUFF_SIZE, &b2_sz);
    if (b1_sz && b2_sz)
    while (1) {
        wait4b2();
        //printf("read b2\n");
        f_read(f, b2, ONE_BUFF_SIZE, &b2_sz);
        if (!b2_sz) {
            wait4end();
            break;
        }
        wait4b1();
        //printf("read b1\n");
        f_read(f, b1, ONE_BUFF_SIZE, &b1_sz);
        if (!b1_sz) {
            wait4end();
            break;
        }
    }
    free(b2);
    free(b1);
    pcm_cleanup();
    printf("Done. Both buffers are deallocated.\n");
e2:
    free(w);
e1:
    free(f);
    return res;
}

int __required_m_api_verion(void) {
    return M_API_VERSION;
}
