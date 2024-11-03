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

typedef struct INFO {
    char INFO[4];
} INFO_t;

typedef struct info {
    char info_id[4];
    uint32_t size;
} info_t;

typedef struct info_desc {
    const char FORB[5];
    const char* desc;
} info_desc_t;

static const info_desc_t info_descs[22] = {
 { "IARL",	"The location where the subject of the file is archived" },
 { "IART",	"The artist of the original subject of the file" },
 { "ICMS",	"The name of the person or organization that commissioned the original subject of the file" },
 { "ICMT",	"General comments about the file or its subject" },
 { "ICOP",	"Copyright information about the file" },
 { "ICRD",	"The date the subject of the file was created" },
 { "ICRP",	"Whether and how an image was cropped" },
 { "IDIM",	"The dimensions of the original subject of the file" },
 { "IDPI",	"Dots per inch settings used to digitize the file" },
 { "IENG",	"The name of the engineer who worked on the file" },
 { "IGNR",	"The genre of the subject" },
 { "IKEY",	"A list of keywords for the file or its subject" },
 { "ILGT",	"Lightness settings used to digitize the file" },
 { "IMED",	"Medium for the original subject of the file" },
 { "INAM",	"Title of the subject of the file (name)" },
 { "IPLT",	"The number of colors in the color palette used to digitize the file" },
 { "IPRD",	"Name of the title the subject was originally intended for" },
 { "ISB",	"Description of the contents of the file (subject)" },
 { "ISFT",	"Name of the software package used to create the file" },
 { "ISRC",	"The name of the person or organization that supplied the original subject of the file" },
 { "ISRF",	"The original form of the material that was digitized (source form)" },
 { "ITCH",	"The name of the technician who digitized the subject file" }
};

inline static const info_desc_t* get_info_desc(const char info[4]) {
    for (size_t i = 0; i < 22; ++i) {
        if (strncmp(info_descs[i].FORB, info, 4) == 0) {
            return info_descs + i;
        }
    }
    return NULL;
}

inline static void hex(const char* buf, UINT rb, FIL* fc) {
    for (unsigned i = 0; i < rb; i += 16) {
        for (unsigned j = 0; j < 16; ++j) {
            if (j + i < rb) {
                fprintf(fc, "0x%02X, ", buf[i + j]);
            } else {
                fprintf(fc, "   ");
            }
            if(j == 7) {
                fprintf(fc, " ");
            }
        }
        fprintf(fc, "\n");
    }
    fprintf(fc, "\n");
}

void process_wav_file(const char* dn, const char* fn, FIL* fh) {
    char ofn[128];
    char vn[128];
    uint8_t buf[256];
    cmd_ctx_t* ctx = get_cmd_ctx();
    FIL* f = (FIL*)malloc(sizeof(FIL));
    snprintf(buf, 256, "%s/%s", dn, fn);
    if (f_open(f, buf, FA_READ) != FR_OK) {
        fprintf(ctx->std_err, "Unable to open file: '%s'\n", buf);
        free(f);
        return;
    }
    wav_t* w = (wav_t*)malloc(sizeof(wav_t));
    UINT rb;
    if (f_read(f, w, sizeof(wav_t), &rb) != FR_OK) {
        fprintf(ctx->std_err, "Unable to read file: '%s'\n", buf);
        goto e1;
    }
    if (strncmp("RIFF", w->RIFF, 4) != 0) {
        fprintf(ctx->std_err, "Unsupported file type: '%s' (RIFF is expected)\n", w->RIFF);
        goto e1;
    }
    if (strncmp("WAVEfmt ", w->WAVEfmt, 8) != 0) {
        fprintf(ctx->std_err, "Unsupported file format: '%s' (WAVEfmt is expected)\n", buf);
        goto e1;
    }
    if (w->h_size != 18 && w->h_size != 16) {
        fprintf(ctx->std_err, "Unsupported header size: %d (16 or 18 are expected)\n", w->h_size);
        goto e1;
    }

    printf("       file name: %s\n", fn);
    printf("       file size: %d\n", w->f_szie + 8);
    printf("     header size: %d\n", w->h_size);
    printf("             pcm: %d\n", w->pcm);
    printf("        channels: %d\n", w->ch);
    printf("            freq: %d Hz\n", w->freq);
    printf("bytes per second: %d (%d KB/s)\n", w->byte_per_second, w->byte_per_second >> 10);
    printf("bytes per sample: %d\n", w->byte_per_sample);
    printf("bits per channel: %d\n"
           " --- \n", w->bit_per_sample);
    if (w->freq != 44100 && w->freq != 22050) {
        fprintf(ctx->std_err, "Unsupported file freq = %d (44100 or 22050 are expected)\n", w->freq);
        goto e1;
    }
    if (w->pcm != 1) {
        fprintf(ctx->std_err, "Unsupported file format: PCM = %d (1 is expected)\n", w->ch);
        goto e1;
    }
    if (w->ch != 1 && w->ch != 2) {
        fprintf(ctx->std_err, "Unsupported number of channels: %d (1 or 2 is expected)\n", w->ch);
        goto e1;
    }
    if (w->bit_per_sample != 8 && w->bit_per_sample != 16) {
        fprintf(ctx->std_err, "Unsupported bit per samples: %d (8 or 18 is expected)\n", w->bit_per_sample);
        goto e1;
    }

    if (w->h_size == 18) {
        f_lseek(&f, 44);
    } else {

    printf("   data chunk id: %c%c%c%c\n", w->data[0], w->data[1], w->data[2], w->data[3]);
    printf(" data chunk size: %d (%d KB)\n"
           " --- \n", w->subchunk_size, w->subchunk_size >> 10);

    if (strncmp(w->data, "LIST", 4) == 0) {
        char* sch = (char*)malloc(w->subchunk_size);
        size_t size;
        if (f_read(f, sch, w->subchunk_size, &size) != FR_OK || size != w->subchunk_size) {
            fprintf(ctx->std_err, "Unexpected end of file: '%s'\n");
            free(sch);
            goto e1;
        }
        INFO_t* ch = sch;
        if (strncmp(ch->INFO, "INFO", 4) != 0) {
            fprintf(ctx->std_err, "Unexpected LIST section in the file: '%s'\n");
            hex(sch, w->subchunk_size, ctx->std_err);
            free(sch);
            goto e1;
        }
        size_t off = sizeof(INFO_t);
        while (off < w->subchunk_size) {
            info_t* ch = (info_t*)(sch + off);
            size_t sz = ch->size;
            printf(" info chunk size: %d\n", sz);
            const info_desc_t* info_desc = get_info_desc(ch->info_id);
            if (!info_desc) {
                printf("   info chunk id: %c%c%c%c (Unknown tag)\n", ch->info_id[0], ch->info_id[1], ch->info_id[2], ch->info_id[3]);
            } else {
                printf("   info chunk id: %s (%s)\n", info_desc->FORB, info_desc->desc);
            }
            off += sizeof(info_t);
            if (sz > 0) {
                char* s = sch + off;
                printf(" info chunk text: %s\n --- \n", s);
                off += sz;
                while (!*(sch + off)) ++off;
            }
            if ((off < w->subchunk_size)) printf("...\n");
            break; // TODO: why it hangs? on next one?
        }
        free(sch);
    }
    }

    snprintf(ofn, 128, "%s.c", fn);
    snprintf(vn, 128, "_%s", fn);
    char* b = vn;
    while(*b) {
        if (*b == '.') *b = '_';
        if (*b == ' ') *b = '_';
        if (*b == '/') *b = '_';
        if (*b == '\\') *b = '_';
        if (*b == '-') *b = '_';
        b++;
    }

    snprintf(buf, 256, "extern const unsigned char %s[];\n", vn);
    UINT sz;
    f_write(fh, buf, strlen(buf), &sz);

    FIL* fc = (FIL*)malloc(sizeof(FIL));
    f_open(fc, ofn, FA_WRITE | FA_CREATE_ALWAYS);
    snprintf(buf, 256, "const unsigned char %s[] = {\n", vn);
    f_write(fc, buf, strlen(buf), &sz);

    while (getch_now() != CHAR_CODE_ESC && !marked_to_exit) {
        if (f_read(f, buf, 256, &sz) != FR_OK || !sz) {
            break;
        }
        if (w->ch == 2) {
            if (w->byte_per_sample == 4) { // Stereo i16, join channels, reduce to i8
                sz >>= 2;
                int16_t* i16 = (int16_t*)buf;
                for (size_t i = 0; i < sz; ++i) {
                    register size_t j = i << 1;
                    buf[i] = ((int32_t)(i16[j]) + i16[j + 1]) >> 9;
                }
            }
            else if (w->byte_per_sample == 2) { // Stereo i8, join channels
                sz >>= 1;
                for (size_t i = 0; i < sz; ++i) {
                    register size_t j = i << 1;
                    buf[i] = ((int16_t)(buf[j]) + buf[j + 1]) >> 1;
                }
            }
        } else {
            if (w->byte_per_sample == 2) { // Mono i16, reduce to i8
                sz >>= 1;
                int16_t* i16 = (int16_t*)buf;
                for (size_t i = 0; i < sz; ++i) {
                    buf[i] = i16[i] >> 8;
                }
            }
            // else Mono i8 (already target)
        }
        if (w->freq == 44100) { // resampling to 22050
            sz >>= 1;
            for (size_t i = 0; i < sz; ++i) {
                buf[i] = buf[i << 1];
            }
        }
        hex(buf, sz, fc);
    }

    snprintf(buf, 256, "};\n", vn);
    f_write(fc, buf, strlen(buf), &sz);
    f_close(fc);
    free(fc);

e1:
    free(w);
    free(f);
}

int main(void) {
    marked_to_exit = false;
    cmd_ctx_t* ctx = get_cmd_ctx();
    if (ctx->argc != 2) {
        fprintf(ctx->std_err,
            "Usage: wav2rom8 [dir]\n"
            "  where [dir] - a folder with .wav files\n"
        );
        return 1;
    }
    int res = 0;
    DIR* d = (DIR*)malloc(sizeof(DIR));
    if (f_opendir(d, ctx->argv[1]) != FR_OK) {
        fprintf(ctx->std_err, "Unable to open '%s' directory\n", ctx->argv[1]);
        res = 2;
        goto c0;
    }
    FIL* fh = (FIL*)malloc(sizeof(FIL));
    f_open(fh, "wav_files.h", FA_WRITE | FA_CREATE_ALWAYS);
    FILINFO* e = (FILINFO*)malloc(sizeof(FILINFO));
    while (f_readdir(d, e) == FR_OK && e->fname[0] != '\0') {
        if (e->fattrib & AM_DIR) continue;
        size_t sz = strlen(e->fname);
        if ( strcmp(".wav", e->fname + sz - 4) == 0 ) {
            process_wav_file(ctx->argv[1], e->fname, fh);
        }
    }
    f_close(fh);
    free(fh);
    free(e);
c0:
    f_closedir(d);
    free(d);
    printf("Done. All buffers are deallocated.\n");
    return res;
}
