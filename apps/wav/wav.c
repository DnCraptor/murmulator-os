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
    if(f_read(f, w, sizeof(wav_t), &rb) != FR_OK) {
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

    printf("      file name: %s\n", ctx->argv[1]);
    printf("      file size: %d\n", w->f_szie + 8);
    printf("    header size: %d\n", w->h_size);
    printf("            pcm: %d\n", w->pcm);
    printf("       channels: %d\n", w->ch);
    printf("           hreq: %d Hz\n", w->freq);
    printf("byts per second: %d\n", w->byte_per_second);
    printf("byts per sample: %d\n", w->byte_per_sample);
    printf("bits per sample: %d\n --- \n", w->bit_per_sample);

    printf("        data id: %c%c%c%c\n", w->data[0], w->data[1], w->data[2], w->data[3]);
    printf("      data size: %d\n", w->subchunk_size);

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
e2:
    free(w);
e1:
    free(f);
    return res;
}

int __required_m_api_verion(void) {
    return M_API_VERSION;
}
