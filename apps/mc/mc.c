#include "m-os-api.h"

static void m_window();
static void redraw_window();
static void draw_cmd_line(int left, int top, char* cmd);

#define PANEL_TOP_Y 0
#define FIRST_FILE_LINE_ON_PANEL_Y (PANEL_TOP_Y + 1)
static uint32_t MAX_HEIGHT, MAX_WIDTH;
static uint8_t PANEL_LAST_Y, F_BTN_Y_POS, CMD_Y_POS, LAST_FILE_LINE_ON_PANEL_Y;
#define TOTAL_SCREEN_LINES MAX_HEIGHT
static char* line;
static char selected_file_path[260]; // TODO: ctx->
static char os_type[160]; 

static volatile bool backspacePressed = false;
static volatile bool enterPressed = false;
static volatile bool plusPressed = false;
static volatile bool minusPressed = false;
static volatile bool ctrlPressed = false;
static volatile bool altPressed = false;
static volatile bool delPressed = false;
static volatile bool f1Pressed = false;
static volatile bool f2Pressed = false;
static volatile bool f3Pressed = false;
static volatile bool f4Pressed = false;
static volatile bool f5Pressed = false;
static volatile bool f6Pressed = false;
static volatile bool f7Pressed = false;
static volatile bool f8Pressed = false;
static volatile bool f9Pressed = false;
static volatile bool f10Pressed = false;
static volatile bool f11Pressed = false;
static volatile bool f12Pressed = false;
static volatile bool tabPressed = false;
static volatile bool spacePressed = false;
static volatile bool escPressed = false;
static volatile bool leftPressed = false;
static volatile bool rightPressed = false;
static volatile bool upPressed = false;
static volatile bool downPressed = false;

// type of F1-F10 function pointer
typedef void (*fn_1_12_ptr)(uint8_t);

#define BTN_WIDTH 8
typedef struct fn_1_12_tbl_rec {
    char pre_mark;
    char mark;
    char name[BTN_WIDTH];
    fn_1_12_ptr action;
} fn_1_12_tbl_rec_t;

#define BTNS_COUNT 12
typedef fn_1_12_tbl_rec_t fn_1_12_tbl_t[BTNS_COUNT];

typedef struct {
    int selected_file_idx;
    int start_file_offset;
    int dir_num;
} indexes_t;

typedef struct file_panel_desc {
    int left;
    int width;
    int files_number;
    char path[256];
    indexes_t indexes[16]; // TODO: some ext. limit logic
    int level;
#if EXT_DRIVES_MOUNT
    bool in_dos;
#endif
} file_panel_desc_t;
static void fill_panel(file_panel_desc_t* p);

static file_panel_desc_t* left_panel;
static file_panel_desc_t* right_panel;
static file_panel_desc_t* psp;

typedef struct color_schema {
    uint8_t BACKGROUND_FIELD_COLOR;
    uint8_t FOREGROUND_FIELD_COLOR;
    uint8_t HIGHLIGHTED_FIELD_COLOR;
    uint8_t BACKGROUND_F1_12_COLOR;
    uint8_t FOREGROUND_F1_12_COLOR;
    uint8_t BACKGROUND_F_BTN_COLOR;
    uint8_t FOREGROUND_F_BTN_COLOR;
    uint8_t BACKGROUND_CMD_COLOR;
    uint8_t FOREGROUND_CMD_COLOR;
    uint8_t BACKGROUND_SEL_BTN_COLOR;
    uint8_t FOREGROUND_SELECTED_COLOR;
    uint8_t BACKGROUND_SELECTED_COLOR;
} color_schema_t;

typedef struct line {
   int8_t off;
   char* txt;
} line_t;

typedef struct lines {
   uint8_t sz;
   uint8_t toff;
   line_t* plns;
} lines_t;

static color_schema_t* pcs;

typedef struct {
    FSIZE_t fsize;   /* File size */
    WORD    fdate;   /* Modified date */
    WORD    ftime;   /* Modified time */
    BYTE    fattrib; /* File attribute */
    char*   pname; //[MAX_WIDTH >> 1];
    int     dir_num;
} file_info_t;

#define MAX_FILES 500

static file_info_t* files_info;
static size_t files_count;

void* memset(void* p, int v, size_t sz) {
    typedef void* (*fn)(void *, int, size_t);
    return ((fn)_sys_table_ptrs[142])(p, v, sz);
}

void* memcpy(void *__restrict dst, const void *__restrict src, size_t sz) {
    typedef void* (*fn)(void *, const void*, size_t);
    return ((fn)_sys_table_ptrs[167])(dst, src, sz);
}

int _init(void) {
    files_count = 0;
    selected_file_path[0] = 0;
    os_type[0] = 0;
}

inline static void m_cleanup() {
    files_count = 0;
}

static void draw_label(int left, int top, int width, char* txt, bool selected, bool highlighted) {
    char line[MAX_WIDTH + 2];
    bool fin = false;
    for (int i = 0; i < width; ++i) {
        if (!fin) {
            if (!txt[i]) {
                fin = true;
                line[i] = ' ';
            } else {
                line[i] = txt[i];
            }
        } else {
            line[i] = ' ';
        }
    }
    line[width] = 0;
    int fgc = selected ? pcs->FOREGROUND_SELECTED_COLOR : highlighted ? pcs->HIGHLIGHTED_FIELD_COLOR : pcs->FOREGROUND_FIELD_COLOR;
    int bgc = selected ? pcs->BACKGROUND_SELECTED_COLOR : pcs->BACKGROUND_FIELD_COLOR;
    draw_text(line, left, top, fgc, bgc);
}


static void draw_panel(int left, int top, int width, int height, char* title, char* bottom) {
    char line[MAX_WIDTH + 2];
    // top line
    for(int i = 1; i < width - 1; ++i) {
        line[i] = 0xCD; // ═
    }
    line[0]         = 0xC9; // ╔
    line[width - 1] = 0xBB; // ╗
    line[width]     = 0;
    draw_text(line, left, top, pcs->FOREGROUND_FIELD_COLOR, pcs->BACKGROUND_FIELD_COLOR); 
    if (title) {
        int sl = strlen(title);
        if (width - 4 < sl) {
            title -= width + 4; // cat title
            sl -= width + 4;
        }
        int title_left = left + (width - sl) / 2;
        sprintf(line, " %s ", title);
        draw_text(line, title_left, top, pcs->FOREGROUND_FIELD_COLOR, pcs->BACKGROUND_FIELD_COLOR);
    }
    // middle lines
    memset(line, ' ', width);
    line[0]         = 0xBA; // ║
    line[width - 1] = 0xBA;
    line[width]     = 0;
    for (int y = top + 1; y < top + height - 1; ++y) {
        draw_text(line, left, y, pcs->FOREGROUND_FIELD_COLOR, pcs->BACKGROUND_FIELD_COLOR);
    }
    // bottom line
    for(int i = 1; i < width - 1; ++i) {
        line[i] = 0xCD; // ═
    }
    line[0]         = 0xC8; // ╚
    line[width - 1] = 0xBC; // ╝
    line[width]     = 0;
    draw_text(line, left, top + height - 1, pcs->FOREGROUND_FIELD_COLOR, pcs->BACKGROUND_FIELD_COLOR);
    if (bottom) {
        int sl = strlen(bottom);
        if (width - 4 < sl) {
            bottom -= width + 4; // cat bottom
            sl -= width + 4;
        } 
        int bottom_left = (width - sl) / 2;
        sprintf(line, " %s ", bottom);
        draw_text(line, bottom_left, top + height - 1, pcs->FOREGROUND_FIELD_COLOR, pcs->BACKGROUND_FIELD_COLOR);
    }
}

static void draw_box(int left, int top, int width, int height, const char* title, const lines_t* plines) {
    draw_panel(left, top, width, height, title, 0);
    int y = top + 1;
    for (int i = y; y < top + height - 1; ++y) {
        draw_label(left + 1, y, width - 2, "", false, false);
    }
    for (int i = 0, y = top + 1 + plines->toff; i < plines->sz; ++i, ++y) {
        const line_t * pl = plines->plns + i;
        uint8_t off;
        if (pl->off < 0) {
            size_t len = strnlen(pl->txt, MAX_WIDTH);
            off = width - 2 > len ? (width - len) >> 1 : 0;
        } else {
            off = pl->off;
        }
        draw_label(left + 1 + off, y, width - 2 - off, pl->txt, false, false);
    }
}

static void do_nothing(uint8_t cmd) {
    snprintf(line, 130, "CMD: F%d", cmd + 1);
    const line_t lns[2] = {
        { -1, "Not yet implemented function" },
        { -1, line }
    };
    const lines_t lines = { 2, 3, lns };
    draw_box((MAX_WIDTH - 60) / 2, 7, 60, 10, "Info", &lines);
    vTaskDelay(1500);
    redraw_window();
}

// TODO:
#define m_info do_nothing
#define save_snap do_nothing
#define m_copy_file do_nothing
#define m_move_file do_nothing
#define m_mk_dir do_nothing
#define m_delete_file do_nothing
#define swap_drives do_nothing
#define mark_to_exit do_nothing
#define switch_mode do_nothing
#define switch_color do_nothing
#define conf_it do_nothing
#define turn_usb_on do_nothing
#define restore_snap do_nothing

static fn_1_12_tbl_t fn_1_12_tbl = {
    ' ', '1', " Help ", m_info,
    ' ', '2', " Snap ", save_snap,
    ' ', '3', " View ", do_nothing,
    ' ', '4', " Edit ", do_nothing,
    ' ', '5', " Copy ", m_copy_file,
    ' ', '6', " Move ", m_move_file,
    ' ', '7', "MkDir ", m_mk_dir,
    ' ', '8', " Del  ", m_delete_file,
    ' ', '9', " Swap ", swap_drives,
    '1', '0', " Exit ", mark_to_exit,
    '1', '1', "EmMODE", switch_mode,
    '1', '2', " B/W  ", switch_color
};

static fn_1_12_tbl_t fn_1_12_tbl_alt = {
    ' ', '1', "Right ", do_nothing,
    ' ', '2', " Conf ", conf_it,
    ' ', '3', " View ", do_nothing,
    ' ', '4', " Edit ", do_nothing,
    ' ', '5', " Copy ", m_copy_file,
    ' ', '6', " Move ", m_move_file,
    ' ', '7', " Find ", do_nothing,
    ' ', '8', " Del  ", m_delete_file,
    ' ', '9', " UpMn ", do_nothing,
    '1', '0', " USB  ", turn_usb_on,
    '1', '1', "EmMODE", switch_mode,
    '1', '2', " B/W  ", switch_color
};

static fn_1_12_tbl_t fn_1_12_tbl_ctrl = {
    ' ', '1', "Eject ", do_nothing,
    ' ', '2', "ReSnap", restore_snap,
    ' ', '3', "Debug ", do_nothing,
    ' ', '4', " Edit ", do_nothing,
    ' ', '5', " Copy ", m_copy_file,
    ' ', '6', " Move ", m_move_file,
    ' ', '7', " Find ", do_nothing,
    ' ', '8', " Del  ", m_delete_file,
    ' ', '9', " Swap ", swap_drives,
    '1', '0', " Exit ", mark_to_exit,
    '1', '1', "EmMODE", switch_mode,
    '1', '2', " B/W  ", switch_color
};

static inline fn_1_12_tbl_t* actual_fn_1_12_tbl() {
    const fn_1_12_tbl_t * ptbl = &fn_1_12_tbl;
    if (altPressed) {
        ptbl = &fn_1_12_tbl_alt;
    } else if (ctrlPressed) {
        ptbl = &fn_1_12_tbl_ctrl;
    }
    return ptbl;
}

static void draw_fn_btn(fn_1_12_tbl_rec_t* prec, int left, int top) {
    char line[10];
    sprintf(line, "       ");
    // 1, 2, 3... button mark
    line[0] = prec->pre_mark;
    line[1] = prec->mark;
    draw_text(line, left, top, pcs->FOREGROUND_F1_12_COLOR, pcs->BACKGROUND_F1_12_COLOR);
    // button
    sprintf(line, prec->name);
    draw_text(line, left + 2, top, pcs->FOREGROUND_F_BTN_COLOR, pcs->BACKGROUND_F_BTN_COLOR);
}

static void bottom_line() {
    for (int i = 0; i < BTNS_COUNT && (i + 1) * BTN_WIDTH < MAX_WIDTH; ++i) {
        const fn_1_12_tbl_rec_t* rec = &(*actual_fn_1_12_tbl())[i];
        draw_fn_btn(rec, i * BTN_WIDTH, F_BTN_Y_POS);
    }
//    draw_text( // TODO: move to pico-vision
//        bk_mode_lns[g_conf.bk0010mode].txt,
//        BTN_WIDTH * 13 + 3,
//        F_BTN_Y_POS,
//        get_color_schema()->FOREGROUND_FIELD_COLOR,
//        get_color_schema()->BACKGROUND_FIELD_COLOR
//    );
//    char buf[4];
//    snprintf(buf, 4, "%s%s", g_conf.is_AY_on ? "A" : " ", g_conf.is_covox_on ? "C" : " ");
//    draw_text( // TODO: move to pico-vision
//        buf,
//        BTN_WIDTH * 13,
//        F_BTN_Y_POS,
//        5, // red
//        0  // black
//    );
    draw_cmd_line(0, CMD_Y_POS, os_type);
}

inline static void update_menu_color() {
 //   const char * m = g_conf.color_mode ? " B/W  " : " Color";
 //   snprintf(fn_1_12_tbl[11].name, BTN_WIDTH, m);
 //   snprintf(fn_1_12_tbl_alt[11].name, BTN_WIDTH, m);
 //   snprintf(fn_1_12_tbl_ctrl[11].name, BTN_WIDTH, m);
    bottom_line();
}

static void redraw_window() {
    m_window();
    fill_panel(left_panel);
    fill_panel(right_panel);
    draw_cmd_line(0, CMD_Y_POS, os_type);
    update_menu_color();
}

inline static bool m_opendir(
	DIR* dp,			/* Pointer to directory object to create */
	const TCHAR* path	/* Pointer to the directory path */
) {
    if (f_opendir(dp, path) != FR_OK) {
        const line_t lns[1] = {
            { -1, "It is not a folder!" }
        };
        const lines_t lines = { 1, 4, lns };
        draw_box((MAX_WIDTH - 60) / 2, 7, 60, 10, "Warning", &lines);
        vTaskDelay(1500);
        redraw_window();
        return false;
    }
    return true;
}

static int m_comp(const file_info_t * e1, const file_info_t * e2) {
    if ((e1->fattrib & AM_DIR) && !(e2->fattrib & AM_DIR)) return -1;
    if (!(e1->fattrib & AM_DIR) && (e2->fattrib & AM_DIR)) return 1;
    return strncmp(e1->pname, e2->pname, MAX_WIDTH >> 1);
}

inline static void m_add_file(FILINFO* fi) {
    if (files_count >= MAX_FILES) {
        // WARN?
        return;
    }
    file_info_t* fp = &files_info[files_count++];
    fp->fattrib = fi->fattrib;
    fp->fdate   = fi->fdate;
    fp->ftime   = fi->ftime;
    fp->fsize   = fi->fsize;
    if (fp->pname) free(fp->pname);
    size_t sz = strlen(fi->fname);
    if (sz > (MAX_WIDTH >> 1)) sz = MAX_WIDTH >> 1;
    fp->pname = malloc(sz);
    strncpy(fp->pname, fi->fname, sz);
}

static void draw_cmd_line(int left, int top, char* cmd) {
    char line[MAX_WIDTH + 2];
    if (cmd) {
        int sl = strlen(cmd);
        snprintf(line, MAX_WIDTH, ">%s", cmd);
        memset(line + sl + 1, ' ', MAX_WIDTH - sl);
    } else {
        memset(line, ' ', MAX_WIDTH); line[0] = '>';
    }
    line[MAX_WIDTH] = 0;
    draw_text(line, left, top, pcs->FOREGROUND_CMD_COLOR, pcs->BACKGROUND_CMD_COLOR);
}

inline static void collect_files(file_panel_desc_t* p) {
    m_cleanup();
#if EXT_DRIVES_MOUNT
    if (p->in_dos) {
        if(!mount_img(p->path, p->indexes[p->level].dir_num)) {
           return;
        }
        qsort (files_info, files_count, sizeof(file_info_t), m_comp);
        return;
    }
#endif
    DIR dir;
    if (!m_opendir(&dir, p->path)) return;
    FILINFO fileInfo;
    while(f_readdir(&dir, &fileInfo) == FR_OK && fileInfo.fname[0] != '\0') {
        m_add_file(&fileInfo);
    }
    f_closedir(&dir);
    qsort (files_info, files_count, sizeof(file_info_t), m_comp);
}

inline static void construct_full_name(char* dst, const char* folder, const char* file) {
    if (strlen(folder) > 1) {
        snprintf(dst, 256, "%s\\%s", folder, file);
    } else {
        snprintf(dst, 256, "\\%s", file);
    }
}

void detect_os_type(const char* path, char* os_type, size_t sz) {
    // TODO:
    strcpy(os_type, path);
}

static void fill_panel(file_panel_desc_t* p) {
    collect_files(p);
    indexes_t* pp = &p->indexes[p->level];
    if (pp->selected_file_idx < FIRST_FILE_LINE_ON_PANEL_Y)
        pp->selected_file_idx = FIRST_FILE_LINE_ON_PANEL_Y;
    if (pp->start_file_offset < 0)
        pp->start_file_offset = 0;
    int y = 1;
    p->files_number = 0;
    int start_file_offset = pp->start_file_offset;
    int selected_file_idx = pp->selected_file_idx;
    if (start_file_offset == 0 && strlen(p->path) > 1) {
        draw_label(p->left + 1, y, p->width - 2, "..", p == psp && selected_file_idx == y, true);
        y++;
        p->files_number++;
    }
    for(int fn = 0; fn < files_count; ++ fn) {
        file_info_t* fp = &files_info[fn];
        if (start_file_offset <= p->files_number && y <= LAST_FILE_LINE_ON_PANEL_Y) {
            char* filename = fp->pname;
            snprintf(line, MAX_WIDTH, "%s\\%s", p->path, fp->pname);
#if EXT_DRIVES_MOUNT
            if (!p->in_dos)
            for (int i = 0; i < 4; ++i) { // mark mounted drived by labels FDD0,FDD1,HDD0...
                if (drives_states[i].path && strncmp(drives_states[i].path, line, MAX_WIDTH) == 0) {
                    snprintf(line, p->width, "%s", fp->name);
                    for (int j = strlen(fp->name); j < p->width - 6; ++j) {
                        line[j] = ' ';
                    }
                    snprintf(line + p->width - 6, 6, "%s", drives_states[i].lbl);
                    filename = line;
                    break;
                }
            }
#endif
            bool selected = p == psp && selected_file_idx == y;
            draw_label(p->left + 1, y, p->width - 2, filename, selected, fp->fattrib & AM_DIR);
            if (
#if EXT_DRIVES_MOUNT
                !p->in_dos &&
#endif
                selected && (fp->fattrib & AM_DIR) == 0
            ) {
                char path[260];
                construct_full_name(path, p->path, fp->pname);
                if (strncmp(selected_file_path, path, 260) != 0) {
                    detect_os_type(path, os_type, 160);
                    strncpy(selected_file_path, path, 260);
                }
                draw_cmd_line(0, CMD_Y_POS, os_type);
            } else if (
#if EXT_DRIVES_MOUNT
              p->in_dos ||
#endif
              selected
            ) {
                os_type[0] = 0;
                draw_cmd_line(0, CMD_Y_POS, 0);
            }
            y++;
        }
        p->files_number++;
    }
    for (; y <= LAST_FILE_LINE_ON_PANEL_Y; ++y) {
        draw_label(p->left + 1, y, p->width - 2, "", false, false);
    }
}

inline static void select_right_panel() {
    psp = &right_panel;
    fill_panel(&left_panel);
    fill_panel(&right_panel);
}

inline static void select_left_panel() {
    psp = &left_panel;
    fill_panel(&left_panel);
    fill_panel(&right_panel);
}

static void m_window() {
    sprintf(line, "SD:%s", left_panel->path);
    draw_panel( 0, PANEL_TOP_Y, MAX_WIDTH / 2, PANEL_LAST_Y + 1, line, 0);
    sprintf(line, "SD:%s", right_panel->path);
    draw_panel(MAX_WIDTH / 2, PANEL_TOP_Y, MAX_WIDTH / 2, PANEL_LAST_Y + 1, line, 0);
}

static inline void work_cycle() {
    // TODO:
    vTaskDelay(10000);
}

inline static void start_manager() {
    m_window();
    select_left_panel();
    update_menu_color();
//        m_info(0); // F1 TODO: ensure it is not too aggressive
    work_cycle();
//    set_start_debug_line(25); // ?? to be removed
}

int main(void) {
    cmd_ctx_t* ctx = get_cmd_ctx();
    if (ctx->argc == 0) {
        fprintf(ctx->std_err, "ATTTENTION! BROKEN EXECUTION CONTEXT [%p]!\n", ctx);
        return -1;
    }
    MAX_WIDTH = get_console_width();
    MAX_HEIGHT = get_console_height();
    PANEL_LAST_Y = CMD_Y_POS - 1;
    TOTAL_SCREEN_LINES = MAX_HEIGHT;
    F_BTN_Y_POS = TOTAL_SCREEN_LINES - 1;
    CMD_Y_POS = F_BTN_Y_POS - 1;
    line = calloc(MAX_WIDTH + 2, 1);
    LAST_FILE_LINE_ON_PANEL_Y = PANEL_LAST_Y - 1;

    left_panel = calloc(1, sizeof(file_panel_desc_t));
    left_panel->width = MAX_WIDTH >> 1;
    strncpy(left_panel->path, "/", 256);
    left_panel->indexes[0].selected_file_idx = FIRST_FILE_LINE_ON_PANEL_Y;
    psp = left_panel;

    right_panel = calloc(1, sizeof(file_panel_desc_t));
    right_panel->left = MAX_WIDTH >> 1;
    right_panel->width = MAX_WIDTH >> 1;
    strncpy(right_panel->path, "/MOS", 256);
    right_panel->indexes[0].selected_file_idx = FIRST_FILE_LINE_ON_PANEL_Y;

    pcs = calloc(1, sizeof(color_schema_t));
    pcs->BACKGROUND_FIELD_COLOR = 1, // Blue
    pcs->FOREGROUND_FIELD_COLOR = 7, // White
    pcs->HIGHLIGHTED_FIELD_COLOR= 5, // LightWhite
    pcs->BACKGROUND_F1_12_COLOR = 0, // Black
    pcs->FOREGROUND_F1_12_COLOR = 7, // White
    pcs->BACKGROUND_F_BTN_COLOR = 3, // Green
    pcs->FOREGROUND_F_BTN_COLOR = 0, // Black
    pcs->BACKGROUND_CMD_COLOR = 0, // Black
    pcs->FOREGROUND_CMD_COLOR = 7, // White
    pcs->BACKGROUND_SEL_BTN_COLOR = 11, // Light Blue
    pcs->FOREGROUND_SELECTED_COLOR = 0, // Black
    pcs->BACKGROUND_SELECTED_COLOR = 11, // Light Blue

    files_info = calloc(sizeof(file_info_t), MAX_FILES);

    start_manager();

    free(line);
    free(right_panel);
    free(left_panel);
    free(pcs);
    for (int i = 0; i < MAX_FILES; ++i) {
        if (files_info[i].pname) free(files_info[i].pname); 
    }
    free(files_info);

    return 0;
}

int __required_m_api_verion(void) {
    return M_API_VERSION;
}
