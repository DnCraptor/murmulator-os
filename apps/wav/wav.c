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

typedef enum state {
    EMPTY = 0,
    LOADING_STARTED,
    FILLED,
    LOCKED,
    PROCESSED
} state_t;

typedef struct chunk {
    char* p;
    volatile size_t size;
    volatile state_t state;
} chunk_t;

volatile static node_t* node;

static char* pcm_end_callback(size_t* size) { // assumed callback is executing on core#1
    chunk_t* chunk = node->data;
    chunk->state = PROCESSED;

    node_t* n = node->next;
    node = n;
    chunk = n->data;
    if (chunk->state != FILLED) {
        while (chunk->state == LOADING_STARTED) {
// waiting for load finished (or failed)
        }
        if (chunk->state != FILLED) {
            return NULL; // no more filled data
        }
    }
    chunk->state = LOCKED;
    *size = chunk->size;
    return chunk->p;
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
    printf("            freq: %d Hz\n", w->freq);
    printf("bytes per second: %d (%d KB/s)\n", w->byte_per_second, w->byte_per_second >> 10);
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
    size_t ONE_BUFF_SIZE = (stat->xMinimumEverFreeBytesRemaining >> 2) & 0xFFFFFE00; // using 3/4 of free continues block adjusted for 512 bytes
    free(stat);

    pcm_setup(w->freq);
    node_t* n1 = malloc(sizeof(node_t));
    node_t* n2 = malloc(sizeof(node_t));
    node_t* n3 = malloc(sizeof(node_t));
    chunk_t* c1 = (chunk_t*)calloc(sizeof(chunk_t), 1);
    chunk_t* c2 = (chunk_t*)calloc(sizeof(chunk_t), 1);
    chunk_t* c3 = (chunk_t*)calloc(sizeof(chunk_t), 1);
    c1->p = (char*)malloc(ONE_BUFF_SIZE);
    c2->p = (char*)malloc(ONE_BUFF_SIZE);
    c3->p = (char*)malloc(ONE_BUFF_SIZE);
    n1->data = c1;
    n2->data = c2;
    n3->data = c3;
    n1->next = n2; // cyclic buffer
    n2->next = n3;
    n3->next = n1;
    n1->prev = n3; // actually not used, may be removed?
    n2->prev = n1;
    n3->prev = n2;
    printf("3 buffers are allocated: %d (%d KB) each\n", ONE_BUFF_SIZE, ONE_BUFF_SIZE >> 10);

    node = n1; // start playing from first node

    // preload data to all buffers
    f_read(f, c1->p, ONE_BUFF_SIZE, &c1->size);
    if (c1->size) {
        c1->state = FILLED;
        pcm_set_buffer(c1->p, w->ch, c1->size, pcm_end_callback);
        f_read(f, c2->p, ONE_BUFF_SIZE, &c2->size);
    }
    if (c2->size) {
        c2->state = FILLED;
        f_read(f, c3->p, ONE_BUFF_SIZE, &c3->size);
    }
    if (c3->size) {
        c3->state = FILLED;
    }

    node_t* l_node = n1; // start loading from first node also

    if (c1->size) while (1) {
        chunk_t* ch = (chunk_t*)l_node->data;
        while (ch->state != PROCESSED) {
            vTaskDelay(1);
        }
        ch->state = LOADING_STARTED;
        f_read(f, ch->p, ONE_BUFF_SIZE, &ch->size);
        if (!ch->size && c1->state != LOCKED && c2->state != LOCKED && c3->state != LOCKED) {
            ch->state = EMPTY;
            break;
        }
        ch->state = ch->size ? FILLED : EMPTY;
        l_node = l_node->next;
    }
    free(c3->p);
    free(c2->p);
    free(c1->p);
    free(c3);
    free(c2);
    free(c1);
    free(n3);
    free(n2);
    free(n1);
    pcm_cleanup();
    printf("Done. All buffers are deallocated.\n");
e2:
    free(w);
e1:
    free(f);
    return res;
}

int __required_m_api_verion(void) {
    return M_API_VERSION;
}
