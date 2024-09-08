/***************************************************************************
 *   Copyright (C) 2008 by Deryabin Andrew   				               *
 *   andrew@it-optima.ru                                                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
MOSAudio_t* new_MOSAudio(struct AYSongInfo *info) {
    MOSAudio_t* aa = (MOSAudio_t*)calloc(1, sizeof(MOSAudio_t));
    aa->songinfo = info;
    aa->started = false;
    return aa;
}

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

static pipe_node_t* pipe2cleanup = 0;
static pipe_node_t* last_created = 0;
static size_t reserve = 0;

static void set_reserve(size_t r) {
    reserve = r;
}

static void try_cleanup_it(void) {
    while (pipe2cleanup->ch->state == PROCESSED) {
        pipe_node_t* next = pipe2cleanup->next;
        if (!next) break; // do not remove last node to have a head
        free(pipe2cleanup->ch->p);
        free(pipe2cleanup->ch);
        // printf("Node deallocated: [%p]->next = [%p]\n", pipe2cleanup, next);
        free(pipe2cleanup);
        pipe2cleanup = next;
    }
}

bool Start(MOSAudio_t* a) {
    printf("Start\n");
    marked_to_exit = false;
    pipe2cleanup = 0;
    last_created = 0;
    cmd_ctx_t* ctx = get_cmd_ctx();

    HeapStats_t* stat = (HeapStats_t*)malloc(sizeof(HeapStats_t));
    vPortGetHeapStats(stat);
     // using 1/8 of free continues block adjusted for 512 bytes
    int ONE_BUFF_SIZE = ((stat->xSizeOfLargestFreeBlockInBytes - reserve) >> 3) & 0xFFFFFE00;
    if (ONE_BUFF_SIZE <= 0) {
        fprintf(ctx->std_err, "Not enouth RAM (%d KB). Try to reboot M-OS and try again...\n", stat->xSizeOfLargestFreeBlockInBytes >> 10);
        goto e3;
    }
    pcm_setup(44100);
    pipe = 0; // pipe to play;
    pipe2cleanup = 0;
    last_created = 0;;

    while (getch_now() != CHAR_CODE_ESC && !marked_to_exit) {
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

        n->ch->size = ay_rendersongbuffer(a->songinfo, n->ch->p, ONE_BUFF_SIZE);
        printf("ay_rendersongbuffer returns: %d\n", n->ch->size);
        if (!n->ch->size) {
            // printf("No more data\n");
            n->ch->state = EMPTY;
            break;
        }
        n->ch->state = FILLED;
        if (pipe == 0) {
            pipe = n;
            pipe2cleanup = n;
            pcm_set_buffer(n->ch->p, 2, n->ch->size >> 1, pcm_end_callback);
        }
        if (last_created) {
            // printf("[%p]->next = [%p] (lc)\n", last_created, n);
            last_created->next = n;
        }
        last_created = n;
        // printf("last_created: [%p] (reassigned)\n", n);
        try_cleanup_it();
    }
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
}

void delete_MOSAudio(MOSAudio_t* aa) {
    free(aa);
}
