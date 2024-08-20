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

typedef struct info {
    char INFO[4];
    char info_id[4];
    uint32_t size;
} info_t;

typedef enum state {
    EMPTY = 0,
    LOADING_STARTED = 1,
    FILLED = 2,
    LOCKED = 3,
    PROCESSED = 4
} state_t;

typedef struct chunk {
    char* p;
    volatile size_t size;
    volatile state_t state;
} chunk_t;

typedef struct pipe_node {
    chunk_t* ch;
    volatile struct pipe_node* next;
} pipe_node_t;

volatile static pipe_node_t* pipe;

static char* pcm_end_callback(size_t* size) { // assumed callback is executing on core#1
    if (!pipe) {
        *size = 0;
        return NULL;
    }
//    printf("top of pipe: [%p]\n", pipe);
    chunk_t* chunk = pipe->ch;
    chunk->state = PROCESSED;
    pipe_node_t* n = pipe->next;
    if (!n) {
//        printf("No more data in the pipe: [%p]\n", pipe);
        pipe = NULL;
        *size = 0;
        return NULL;
    }
    pipe = n;
    chunk = n->ch;
//    printf("switch pipe to: [%p] chunk: [%p] chunk->state: %d chunk->size: %d\n", n, n->ch, chunk->state, chunk->size);
    while (chunk->state != FILLED) {
//        printf("chunk->state: %d\n", chunk->state);
    }
    *size = chunk->size >> 1;
    chunk->state = LOCKED;
    return chunk->p;
}

static void convert2int16buff(chunk_t* n_ch, chunk_t* ch) {
    while (n_ch->state != PROCESSED && n_ch->state != EMPTY) {
    //    vTaskDelay(1);
    }
    n_ch->state = LOADING_STARTED;
    uint8_t* p = ch->p;
    int16_t* np = n_ch->p;
    size_t half = ch->size >> 1;
    for (size_t i = half; i < ch->size; ++i) {
        np[i - half] = (int16_t)p[i] << 7;
    }
    np = ch->p; // from remaining part in back order
    for (int i = half - 1; i >= 0; --i) {
        np[i] = (int16_t)p[i] << 7;
    }
    n_ch->state = FILLED;
}

static pipe_node_t* pipe2cleanup = 0;
static pipe_node_t* last_created = 0;;

static void try_cleanup_it(void) {
    if (!pipe2cleanup) pipe2cleanup = pipe;
    while (pipe2cleanup && pipe2cleanup->ch->state == PROCESSED) {
        pipe_node_t* next = pipe2cleanup->next;
        free(pipe2cleanup->ch->p);
        free(pipe2cleanup->ch);
        // printf("Node deallocated: [%p]->next = [%p]\n", pipe2cleanup, next);
        free(pipe2cleanup);
        pipe2cleanup = next;
    }
}

int main(void) {
    cmd_ctx_t* ctx = get_cmd_ctx();
    if (ctx->argc < 2 || ctx->argc > 3) {
e0:
        fprintf(ctx->std_err,
            "Usage: wav [file] [bytes]\n"
            "  where [bytes] - optional param to reserve some RAM for other applications (512 by default).\n"
        );
        return 1;
    }
    int reserve = 512;
    if (ctx->argc == 3) {
        reserve = atoi(ctx->argv[2]);
        if (!reserve) {
            goto e0;
        }
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
    printf("bits per channel: %d\n"
           " --- \n", w->bit_per_sample);

    printf("   data chunk id: %c%c%c%c\n", w->data[0], w->data[1], w->data[2], w->data[3]);
    printf(" data chunk size: %d (%d KB)\n"
           " --- \n", w->subchunk_size, w->subchunk_size >> 10);

    if (strncmp(w->data, "LIST", 4) == 0) {
        char* sch = (char*)malloc(w->subchunk_size);
        size_t size;
        if (f_read(f, sch, w->subchunk_size, &size) != FR_OK || size != w->subchunk_size) {
            fprintf(ctx->std_err, "Unexpected end of file: '%s'\n");
            res = 10;
            free(sch);
            goto e2;
        }
        size_t off = 0;
        info_t* ch = sch;
        if (strncmp(ch->INFO, "INFO", 4) != 0) {
            fprintf(ctx->std_err, "Unexpected LIST section in the file: '%s'\n");
            res = 11;
            free(sch);
            goto e2;
        }
info:
        ch = sch + off;
        printf("   info chunk id: %c%c%c%c\n", ch->info_id[0], ch->info_id[1], ch->info_id[2], ch->info_id[3]);
        printf(" info chunk size: %d\n", ch->size);
        off = sizeof(info_t);;
        if (ch->size > 0) {
            char* buff = malloc(ch->size + 1);
            snprintf(buff, ch->size + 1, "%s", (char*)ch + off);
            printf(" info chunk text: %s\n"
                   " --- \n", buff);
            free(buff);
            off += ch->size;
        }
        if (off < w->subchunk_size) {
            goto info;
        }
        free(sch);
    }

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
    if (w->bit_per_sample != 8 && w->bit_per_sample != 16) {
        fprintf(ctx->std_err, "Unsupported bit per samples: %d (8 or 18 is expected)\n", w->bit_per_sample);
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
     // using 1/8 of free continues block adjusted for 512 bytes
    int ONE_BUFF_SIZE = ((stat->xSizeOfLargestFreeBlockInBytes - reserve) >> 3) & 0xFFFFFE00;
    if (ONE_BUFF_SIZE <= 0) {
        fprintf(ctx->std_err, "Not enouth RAM (%d KB). Try to reboot M-OS and try again...\n", stat->xSizeOfLargestFreeBlockInBytes >> 10);
        res = 9;
        goto e3;
    }

    pcm_setup(w->freq);
    pipe = 0; // pipe to play;
    pipe2cleanup = 0;
    last_created = 0;;

    while (1) {
        vPortGetHeapStats(stat);
        while (stat->xSizeOfLargestFreeBlockInBytes < ONE_BUFF_SIZE + reserve) { // some msg? or break;
        //    vTaskDelay(1);
            try_cleanup_it();
            vPortGetHeapStats(stat);
        }
        pipe_node_t* n = calloc(sizeof(pipe_node_t), 1);
        // printf("Node allocated: [%p]\n", n);
        n->ch = (chunk_t*)calloc(sizeof(chunk_t), 1);
        n->ch->p = (char*)malloc(ONE_BUFF_SIZE);
        n->ch->state = LOADING_STARTED;
        if (f_read(f, n->ch->p, ONE_BUFF_SIZE, &n->ch->size) != FR_OK || !n->ch->size) {
            // printf("No more data\n");
            n->ch->state = EMPTY;
            break;
        }
        if (w->bit_per_sample == 8) { // convert from uint8_t to int16_t
            vPortGetHeapStats(stat);
            while (stat->xSizeOfLargestFreeBlockInBytes < ONE_BUFF_SIZE + reserve) { // some msg? or break;
            //    vTaskDelay(1);
                try_cleanup_it();
                vPortGetHeapStats(stat);
            }
            pipe_node_t* n2 = calloc(sizeof(pipe_node_t), 1);
            // printf("Node(2) allocated: [%p]\n", n);
            n2->ch = (chunk_t*)calloc(sizeof(chunk_t), 1);
            n2->ch->p = (char*)malloc(ONE_BUFF_SIZE);
            n2->ch->size = n->ch->size;
            convert2int16buff(n2->ch, n->ch);
            // printf("[%p]->next = [%p] (n2)\n", n, n2);
            n->next = n2;
        }
        n->ch->state = FILLED;
        if (pipe == 0) {
            pipe = n;
            pipe2cleanup = n;
            pcm_set_buffer(n->ch->p, w->ch, n->ch->size >> 1, pcm_end_callback);
        }
        if (last_created) {
            // printf("[%p]->next = [%p] (lc)\n", last_created, n);
            last_created->next = n;
        }
        if (w->bit_per_sample == 8) { // actually n->next == n2 in this case
            last_created = n->next;
        } else {
            last_created = n;
        }
        // printf("last_created: [%p] (reassigned)\n", n);
        try_cleanup_it();
    }
    if (!pipe2cleanup) pipe2cleanup = pipe;
    while (pipe2cleanup) {
        // printf("Cleanup (finally)\n");
        if(pipe2cleanup->ch->state != LOCKED) {
        //    vTaskDelay(1);
            continue;
        }
        pipe_node_t* next = pipe2cleanup->next;
        free(pipe2cleanup->ch->p);
        free(pipe2cleanup->ch);
        free(pipe2cleanup);
        pipe2cleanup = next;
    }

    pcm_cleanup();
    printf("Done. All buffers are deallocated.\n");
e3:
    free(stat);
e2:
    free(w);
e1:
    free(f);
    return res;
}

int __required_m_api_verion(void) {
    return M_API_VERSION;
}
