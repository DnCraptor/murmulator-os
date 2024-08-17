#include "m-os-api.h"
#include "m-os-api-sdtfn.h"

// #define DEBUG 1

const char CD[] = "CD";
const char TEMP[] = "TEMP";
const char _mc_con[] = ".mc.con";
const char _mc_res[] = ".mc.res";
const char _cmd_history[] = ".cmd_history";

static void m_window();
static void redraw_window();
static void draw_cmd_line(void);
static void bottom_line();
static void construct_full_name(string_t* dst, const char* folder, const char* file);
static void construct_full_name_s(string_t* dst, const string_t* folder, const string_t* file);
static bool m_prompt(const char* txt);
static void no_selected_file();
static bool cmd_enter(cmd_ctx_t* ctx);
static void enter_pressed();

#define PANEL_TOP_Y 0
#define FIRST_FILE_LINE_ON_PANEL_Y (PANEL_TOP_Y + 1)
static uint32_t MAX_HEIGHT, MAX_WIDTH;
static uint8_t PANEL_LAST_Y, F_BTN_Y_POS, CMD_Y_POS, LAST_FILE_LINE_ON_PANEL_Y;
#define TOTAL_SCREEN_LINES MAX_HEIGHT
static char* line;

static volatile uint32_t lastCleanableScanCode;
static volatile uint32_t lastSavedScanCode;
static bool hidePannels = false;

typedef enum sort_type {
    BY_NAME_ASC = 0,
    BY_NAME_DESC,
    BY_EXT_ASC,
    BY_EXT_DESC,
    BY_SIZE_ASC,
    BY_SIZE_DESC,
    BY_DATE_ASC,
    BY_DATE_DESC,
    BY_NOTHING
} sort_type_t;

static volatile bool ctrlPressed = false;
static volatile bool altPressed = false;
static volatile bool leftPressed = false;
static volatile bool rightPressed = false;
static volatile bool upPressed = false;
static volatile bool downPressed = false;

// TODO:
static bool is_dendy_joystick = true;
#define DPAD_STATE_DELAY 200
static int nespad_state_delay = DPAD_STATE_DELAY;
static uint8_t nespad_state, nespad_state2;
static bool mark_to_exit_flag = false;

static string_t* s_cmd = 0;

inline static void nespad_read() {
    nespad_stat(&nespad_state, &nespad_state2);
}

inline static void scan_code_processed() {
  if (lastCleanableScanCode) {
      lastSavedScanCode = lastCleanableScanCode;
  }
  lastCleanableScanCode = 0;
}

static void scan_code_cleanup() {
  lastSavedScanCode = 0;
  lastCleanableScanCode = 0;
}

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
} indexes_t;

typedef struct file_panel_desc {
    int left;
    int width;
    int files_number;
    string_t* s_path;
    indexes_t indexes[16]; // TODO: some ext. limit logic
    int level;
    sort_type_t sort_type;
} file_panel_desc_t;
static void fill_panel(file_panel_desc_t* p);
static void collect_files(file_panel_desc_t* p);

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
    string_t* s_name;
    FSIZE_t fsize;  /* File size */
	WORD	fdate;  /* Modified date */
	WORD	ftime;  /* Modified time */
    BYTE    fattr;  /* File attribute */
} file_info_t;
static array_t* files_info_arr = NULL; // of file_info_t*

static file_info_t* new_file_info(FILINFO* fi) {
    file_info_t* res = (file_info_t*)calloc(sizeof(file_info_t), 1);
    res->s_name = new_string_cc(fi->fname);
    res->fsize = fi->fsize;
    res->fdate = fi->fdate;
    res->ftime = fi->ftime;
    res->fattr = fi->fattrib;
    return res;
}

static file_info_t* fi_allocator(void) {
    file_info_t* res = (file_info_t*)calloc(sizeof(file_info_t), 1);
    res->s_name = new_string_v();
    return res;
}

static void fi_deallocator(file_info_t* p) {
    delete_string(p->s_name);
    free(p);
}

static file_info_t* selected_file(file_panel_desc_t* p, bool with_refresh);

int _init(void) {
    hidePannels = false;

    ctrlPressed = false;
    altPressed = false;
    leftPressed = false;
    rightPressed = false;
    upPressed = false;
    downPressed = false;

    is_dendy_joystick = true;
    nespad_state_delay = DPAD_STATE_DELAY;
    nespad_state = nespad_state2 = 0;
    mark_to_exit_flag = false;
    scan_code_cleanup();
}

inline static void m_cleanup() {
    array_resize(files_info_arr, 0);
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
    if (hidePannels) return;
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
        snprintf(line, MAX_WIDTH, " %s ", title);
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
        snprintf(line, MAX_WIDTH, " %s ", bottom);
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
    snprintf(line, MAX_WIDTH, "CMD: F%d", cmd + 1);
    const line_t lns[2] = {
        { -1, "Not yet implemented function" },
        { -1, line }
    };
    const lines_t lines = { 2, 3, lns };
    draw_box((MAX_WIDTH - 60) / 2, 7, 60, 10, "Info", &lines);
    vTaskDelay(1500);
    redraw_window();
}

static file_info_t* selected_file(file_panel_desc_t* p, bool with_refresh) {
    int start_file_offset = p->indexes[p->level].start_file_offset;
    int selected_file_idx = p->indexes[p->level].selected_file_idx;
    if (selected_file_idx == FIRST_FILE_LINE_ON_PANEL_Y && start_file_offset == 0 && p->s_path->size > 1) {
        return 0;
    }
    if (with_refresh) {
        collect_files(p);
    }
    int y = 1;
    int files_number = 0;
    if (start_file_offset == 0 && p->s_path->size > 1) {
        y++;
        files_number++;
    }
    for(int fn = 0; fn < files_info_arr->size; ++ fn) {
        file_info_t* fp = array_get_at(files_info_arr, fn);
        if (start_file_offset <= files_number && y <= LAST_FILE_LINE_ON_PANEL_Y) {
            if (selected_file_idx == y) {
                return fp;
            }
            y++;
        }
        files_number++;
    }
    return 0; // ?? what a case?
}

static FRESULT m_unlink_recursive(char * path) {
    DIR* pdir = (DIR*)malloc(sizeof(DIR));
    FRESULT res = f_opendir(pdir, path);
    if (res != FR_OK) {
        free(pdir);
        return res;
    }
    FILINFO* pfileInfo = (FILINFO*) malloc(sizeof(FILINFO));
    string_t* s_path = new_string_v();
    while(f_readdir(pdir, pfileInfo) == FR_OK && pfileInfo->fname[0] != '\0') {
        construct_full_name(s_path, path, pfileInfo->fname); // TODO: s_name (modify ff)
        if (pfileInfo->fattrib & AM_DIR) {
            res = m_unlink_recursive(s_path->p);
        } else {
            res = f_unlink(s_path->p);
        }
        if (res != FR_OK) break;
    }
    delete_string(s_path);
    free(pfileInfo);
    f_closedir(pdir);
    free(pdir);
    res = f_unlink(path);
    return res;
}

static void m_delete_file(uint8_t cmd) {
#if EXT_DRIVES_MOUNT
    if (psp->in_dos) {
        // TODO:
        do_nothing(cmd);
        return;
    }
#endif
    file_info_t* fp = selected_file(psp, true);
    if (!fp) {
       no_selected_file();
       return;
    }
    string_t* s_path = new_string_cc("Remove '");
    string_push_back_cs(s_path, fp->s_name);
    string_push_back_cc(s_path, fp->fattr & AM_DIR ? "' folder?" : "' file?");
    if (m_prompt(s_path->p)) {
        construct_full_name_s(s_path, psp->s_path, fp->s_name);
        FRESULT result = fp->fattr & AM_DIR ? m_unlink_recursive(s_path->p) : f_unlink(s_path->p);
        if (result != FR_OK) {
            size_t width = MAX_WIDTH > 60 ? 60 : 40;
            size_t x = (MAX_WIDTH - width) >> 1;
            snprintf(line, width - 2, "FRESULT: %d", result);
            const line_t lns[3] = {
                { -1, "Unable to delete selected item!" },
                { -1, s_path->p },
                { -1, line }
            };
            const lines_t lines = { 3, 2, lns };
            draw_box(x, 7, width, 10, "Error", &lines);
            sleep_ms(2500);
        } else {
            psp->indexes[psp->level].selected_file_idx--;
        }
    }
    delete_string(s_path);
    redraw_window();    
}

inline static FRESULT m_copy(char* path, char* dest) {
    FIL* pfile1 = (FIL*)malloc(sizeof(FIL));
    FRESULT result = f_open(pfile1, path, FA_READ);
    if (result != FR_OK) {
        free(pfile1);
        return result;
    }
    FIL* pfile2 = (FIL*)malloc(sizeof(FIL));
    result = f_open(pfile2, dest, FA_WRITE | FA_CREATE_ALWAYS);
    if (result != FR_OK) {
        free(pfile1);
        free(pfile2);
        f_close(pfile1);
        return result;
    }
    UINT br;
    void* buff = malloc(512);
    do {
        result = f_read(pfile1, buff, 512, &br);
        if (result != FR_OK || br == 0) break;
        UINT bw;
        f_write(pfile2, buff, br, &bw);
        if (result != FR_OK) break;
    } while (br);
    free(buff);
    free(pfile1);
    free(pfile2);
    f_close(pfile1);
    f_close(pfile2);
    return result;
}

static FRESULT m_copy_recursive(const char* path, const char* dest) {
    DIR* pdir = (DIR*)malloc(sizeof(DIR));
    FRESULT res = f_opendir(pdir, path);
    if (res != FR_OK) {
        free(pdir);
        return res;
    }
    res = f_mkdir(dest);
    if (res != FR_OK) {
        free(pdir);
        return res;
    }
    FILINFO* pfileInfo = (FILINFO*) malloc(sizeof(FILINFO));
    string_t* s_path = new_string_v();
    string_t* s_dest = new_string_v();
    while(f_readdir(pdir, pfileInfo) == FR_OK && pfileInfo->fname[0] != '\0') {
        construct_full_name(s_path, path, pfileInfo->fname);
        construct_full_name(s_dest, dest, pfileInfo->fname);
        if (pfileInfo->fattrib & AM_DIR) {
            res = m_copy_recursive(s_path->p, s_dest->p);
        } else {
            res = m_copy(s_path->p, s_dest->p);
        }
        if (res != FR_OK) break;
    }
    free(s_dest);
    free(s_path);
    free(pfileInfo);
    f_closedir(pdir);
    free(pdir);
    return res;
}

static void draw_button(int left, int top, int width, const char* txt, bool selected) {
    int len = strlen(txt);
    if (len > 39) return;
    char tmp[40];
    int start = (width - len) / 2;
    for (int i = 0; i < start; ++i) {
        tmp[i] = ' ';
    }
    bool fin = false;
    int j = 0;
    for (int i = start; i < width; ++i) {
        if (!fin) {
            if (!txt[j]) {
                fin = true;
                tmp[i] = ' ';
            } else {
                tmp[i] = txt[j++];
            }
        } else {
            tmp[i] = ' ';
        }
    }
    tmp[width] = 0;
    draw_text(tmp, left, top, pcs->FOREGROUND_F_BTN_COLOR, selected ? pcs->BACKGROUND_SEL_BTN_COLOR : pcs->BACKGROUND_F_BTN_COLOR);
}

static bool m_prompt(const char* txt) {
    const line_t lns[1] = {
        { -1, txt },
    };
    const lines_t lines = { 1, 2, lns };
    size_t width = MAX_WIDTH > 60 ? 60 : 40;
    size_t shift = MAX_WIDTH > 60 ? 10 : 0;
    size_t x = (MAX_WIDTH - width) >> 1;
    draw_box(x, 7, width, 10, "Are you sure?", &lines);
    bool yes = true;
    draw_button(x + shift + 6, 12, 11, "Yes", yes);
    draw_button(x + shift + 25, 12, 10, "No", !yes);
    while(1) {
        bool tabPressed = false;
        char c = getch_now();
        if (c) {
            if (c == CHAR_CODE_ENTER) {
                scan_code_cleanup();
                return yes;
            }
            if (c == CHAR_CODE_ESC) {
                scan_code_cleanup();
                return false;
            }
            if (c == CHAR_CODE_TAB) {
                tabPressed = true;
            }
        }

        if (is_dendy_joystick) {
            if (is_dendy_joystick) nespad_read();
            if (nespad_state_delay > 0) {
                nespad_state_delay--;
                sleep_ms(1);
            }
            else {
                nespad_state_delay = DPAD_STATE_DELAY;
                if (nespad_state & DPAD_A) {
                    return yes;
                }
                if (nespad_state & DPAD_B) {
                    return false;
                }
                if(nespad_state & DPAD_UP) {
                    upPressed = true;
                } else if(nespad_state & DPAD_DOWN) {
                    downPressed = true;
                } else if (nespad_state & DPAD_LEFT) {
                    leftPressed = true;
                } else if (nespad_state & DPAD_RIGHT) {
                    rightPressed = true;
                } else if (nespad_state & DPAD_SELECT) {
                    tabPressed = true;
                }
            }
        }
        if (tabPressed || leftPressed || rightPressed) { // TODO: own msgs cycle
            yes = !yes;
            draw_button(x + shift + 6, 12, 11, "Yes", yes);
            draw_button(x + shift + 25, 12, 10, "No", !yes);
            tabPressed = leftPressed = rightPressed = false;
            scan_code_cleanup();
        }
    }
    __builtin_unreachable;
}

static void no_selected_file() {
    const line_t lns[1] = {
        { -1, "Pls. select some file for this action" },
    };
    const lines_t lines = { 1, 3, lns };
    draw_box((MAX_WIDTH - 60) / 2, 7, 60, 10, "Info", &lines);
    sleep_ms(1500);
    redraw_window();
}

static void m_copy_file(uint8_t cmd) {
#if EXT_DRIVES_MOUNT
    if (psp->in_dos) {
        // TODO:
        do_nothing(cmd);
        return;
    }
#endif
    file_info_t* fp = selected_file(psp, true);
    if (!fp) {
       no_selected_file();
       return;
    }
    file_panel_desc_t* dsp = psp == left_panel ? right_panel : left_panel;
    string_t* s_path = new_string_cc("Copy '");
    string_push_back_cs(s_path, fp->s_name);
    string_push_back_cc(s_path, fp->fattr & AM_DIR ? "' folder to '" : "' file to '");
    string_push_back_cs(s_path, dsp->s_path);
    string_push_back_cc(s_path, "'?");
    if (m_prompt(s_path->p)) { // TODO: ask name
        construct_full_name_s(s_path, psp->s_path, fp->s_name);
        string_t* s_dest = new_string_v();
        construct_full_name_s(s_dest, dsp->s_path, fp->s_name);
        FRESULT result = fp->fattr & AM_DIR ? m_copy_recursive(s_path->p, s_dest->p) : m_copy(s_path->p, s_dest->p);
        if (result != FR_OK) {
            size_t width = MAX_WIDTH > 60 ? 60 : 40;
            size_t x = (MAX_WIDTH - width) >> 1;
            snprintf(line, width - 2, "FRESULT: %d", result);
            const line_t lns[3] = {
                { -1, "Unable to copy selected item!" },
                { -1, s_path->p },
                { -1, line }
            };
            const lines_t lines = { 3, 2, lns };
            draw_box(x, 7, width, 10, "Error", &lines);
            sleep_ms(2500);
        }
        delete_string(s_dest);
    }
    delete_string(s_path);
    redraw_window();
}

static void turn_usb_off(uint8_t cmd);
static void turn_usb_on(uint8_t cmd);

static void mark_to_exit(uint8_t cmd) {
    mark_to_exit_flag = true;
}

static void m_info(uint8_t cmd) {
    line_t plns[2] = {
        { 1, " It is ZX Murmulator OS Commander" },
        { 1, "tba" }
    };
    lines_t lines = { 2, 0, plns };
    draw_box(5, 2, MAX_WIDTH - 15, MAX_HEIGHT - 6, "Help", &lines);
    nespad_state_delay = DPAD_STATE_DELAY;
    char c = 0;
    while(c != CHAR_CODE_ESC && c != CHAR_CODE_ENTER) {
        if (is_dendy_joystick) {
            if (is_dendy_joystick) nespad_read();
            if (nespad_state && !(nespad_state & DPAD_START) && !(nespad_state & DPAD_SELECT)) {
                nespad_state_delay = DPAD_STATE_DELAY;
                break;
            }
        }
        c = getch_now();
    }
    redraw_window();
}

static void m_mk_dir(uint8_t cmd) {
#if EXT_DRIVES_MOUNT
    if (psp->in_dos) {
        // TODO:
        do_nothing(cmd);
        return;
    }
#endif
    string_t* s_dir = new_string_v();
    construct_full_name(s_dir, psp->s_path->p, "");
    int ox = graphics_con_x();
    int oy = graphics_con_y();
    int x = 4 + s_dir->size;
    int y = MAX_HEIGHT / 2 - 1;
    int width = MAX_WIDTH - 8;
    graphics_set_con_pos(x, y);
    draw_panel(2, y - 2, width + 4, 5, "DIR NAME", 0);
    draw_label(4, y, width, s_dir->p, true, true);
    while(1) {
        char c = getch();
        if (c == CHAR_CODE_ESC) {
            scan_code_cleanup();
            redraw_window();
            return;
        }
        if (c == CHAR_CODE_BS) {
            scan_code_cleanup();
            if (!s_dir->size) {
                blimp(10, 5);
                continue;
            }
            graphics_set_con_pos(--x, y);
            string_resize(s_dir, s_dir->size - 1);
            draw_label(4, y, width, s_dir->p, true, true);
        }
        if (c == CHAR_CODE_ENTER) {
            break;
        }
        uint8_t sc = (uint8_t)lastCleanableScanCode;
        scan_code_cleanup();
        if (!sc || sc >= 0x80) continue;
       // char c = scan_code_2_cp866[sc]; // TODO: shift, caps lock, alt, rus...
        if (!c) continue;
        if (s_dir->size >= width) {
            blimp(10, 5);
            continue;
        }
        string_push_back_c(s_dir, c);
        graphics_set_con_pos(++x, y);
        draw_label(4, y, width, s_dir->p, true, true);
    }
    if (s_dir->size) {
        f_mkdir(s_dir->p);
    }
    graphics_set_con_pos(ox, oy);
    scan_code_cleanup();
    redraw_window();
}

static void m_move_file(uint8_t cmd) {
#if EXT_DRIVES_MOUNT
    if (psp->in_dos) {
        // TODO:
        do_nothing(cmd);
        return;
    }
#endif
    file_info_t* fp = selected_file(psp, true);
    if (!fp) {
       no_selected_file();
       return;
    }
    file_panel_desc_t* dsp = psp == left_panel ? right_panel : left_panel;
    string_t* s_path = new_string_cc("Move '");
    string_push_back_cs(s_path, fp->s_name);
    string_push_back_cc(s_path, fp->fattr & AM_DIR ? "' folder to '" : "' file to '");
    string_push_back_cs(s_path, dsp->s_path);
    string_push_back_cc(s_path, "'?");
    if (m_prompt(s_path->p)) { // TODO: ask name
        construct_full_name_s(s_path, psp->s_path, fp->s_name);
        string_t* s_dest = new_string_v();
        construct_full_name_s(s_dest, dsp->s_path, fp->s_name);
        FRESULT result = f_rename(s_path->p, s_dest->p);
        if (result != FR_OK) {
            size_t width = MAX_WIDTH > 60 ? 60 : 40;
            size_t x = (MAX_WIDTH - width) >> 1;
            snprintf(line, width - 2, "FRESULT: %d", result);
            const line_t lns[3] = {
                { -1, "Unable to move selected item!" },
                { -1, s_path->p },
                { -1, line }
            };
            const lines_t lines = { 3, 2, lns };
            draw_box(x, 7, width, 10, "Error", &lines);
            sleep_ms(2500);
        }
        delete_string(s_dest);
    }
    delete_string(s_path);
    redraw_window();
}

void m_view(uint8_t nu) {
    if (hidePannels) return;
    file_info_t* fp = selected_file(psp, true);
    if (!fp) return; // warn?
    if (fp->fattr & AM_DIR) {
        enter_pressed();
        return;
    }
    string_replace_cs(s_cmd, "mcview \"");
    if (psp->s_path->size > 1)
        string_push_back_cs(s_cmd, psp->s_path);
    string_push_back_c(s_cmd, '/');
    string_push_back_cs(s_cmd, fp->s_name);
    string_push_back_c(s_cmd, '"');
    mark_to_exit_flag = cmd_enter(get_cmd_ctx());
}

void m_edit(uint8_t nu) {
    if (hidePannels) return;
    file_info_t* fp = selected_file(psp, true);
    if (!fp) return; // warn?
    if (fp->fattr & AM_DIR) {
        enter_pressed();
        return;
    }
    string_replace_cs(s_cmd, "mcedit \"");
    if (psp->s_path->size > 1)
        string_push_back_cs(s_cmd, psp->s_path);
    string_push_back_c(s_cmd, '/');
    string_push_back_cs(s_cmd, fp->s_name);
    string_push_back_c(s_cmd, '"');
    mark_to_exit_flag = cmd_enter(get_cmd_ctx());
}

static void m_sort_by_name(uint8_t n) {
    if (psp->sort_type != BY_NAME_ASC) psp->sort_type = BY_NAME_ASC;
    else if (psp->sort_type == BY_NAME_ASC) psp->sort_type = BY_NAME_DESC;
    else psp->sort_type = BY_NAME_ASC;
    fill_panel(psp);
}
static void m_sort_by_ext(uint8_t n) {
    if (psp->sort_type != BY_EXT_ASC) psp->sort_type = BY_EXT_ASC;
    else if (psp->sort_type == BY_EXT_ASC) psp->sort_type = BY_EXT_DESC;
    else psp->sort_type = BY_EXT_ASC;
    fill_panel(psp);
}
static void m_sort_by_size(uint8_t n) {
    if (psp->sort_type != BY_SIZE_ASC) psp->sort_type = BY_SIZE_ASC;
    else if (psp->sort_type == BY_SIZE_ASC) psp->sort_type = BY_SIZE_DESC;
    else psp->sort_type = BY_SIZE_ASC;
    fill_panel(psp);
}
static void m_sort_by_nothing(uint8_t n) {
    if (psp->sort_type != BY_NOTHING) psp->sort_type = BY_NOTHING;
    else psp->sort_type = BY_NAME_ASC;
    fill_panel(psp);
}
static void m_sort_by_date(uint8_t n) {
    if (psp->sort_type != BY_DATE_ASC) psp->sort_type = BY_DATE_ASC;
    else if (psp->sort_type == BY_DATE_ASC) psp->sort_type = BY_DATE_DESC;
    else psp->sort_type = BY_DATE_ASC;
    fill_panel(psp);
}

static fn_1_12_tbl_t fn_1_12_tbl = {
    ' ', '1', " Help ", m_info,
    ' ', '2', "      ", do_nothing,
    ' ', '3', " View ", m_view,
    ' ', '4', " Edit ", m_edit,
    ' ', '5', " Copy ", m_copy_file,
    ' ', '6', " Move ", m_move_file,
    ' ', '7', "MkDir ", m_mk_dir,
    ' ', '8', " Del  ", m_delete_file,
    ' ', '9', "      ", do_nothing,
    '1', '0', " Exit ", mark_to_exit,
    '1', '1', "      ", do_nothing,
    '1', '2', "      ", do_nothing
};

static fn_1_12_tbl_t fn_1_12_tbl_alt = {
    ' ', '1', "      ", do_nothing,
    ' ', '2', "      ", do_nothing,
    ' ', '3', " View ", m_view,
    ' ', '4', " Edit ", m_edit,
    ' ', '5', " Copy ", m_copy_file,
    ' ', '6', " Move ", m_move_file,
    ' ', '7', "      ", do_nothing,
    ' ', '8', " Del  ", m_delete_file,
    ' ', '9', "      ", do_nothing,
    '1', '0', " USB  ", turn_usb_on,
    '1', '1', "      ", do_nothing,
    '1', '2', "      ", do_nothing
};

static fn_1_12_tbl_t fn_1_12_tbl_ctrl = {
    ' ', '1', "      ", do_nothing,
    ' ', '2', "      ", do_nothing,
    ' ', '3', " Name ", m_sort_by_name,
    ' ', '4', " Ext  ", m_sort_by_ext,
    ' ', '5', "      ", do_nothing,
    ' ', '6', " Size ", m_sort_by_size,
    ' ', '7', " Usrot", m_sort_by_nothing,
    ' ', '8', " Date ", m_sort_by_date,
    ' ', '9', "      ", do_nothing,
    '1', '0', "      ", do_nothing,
    '1', '1', "      ", do_nothing,
    '1', '2', "      ", do_nothing
};

static void turn_usb_off(uint8_t cmd) {
    if (tud_msc_ejected()) return;
    usb_driver(false);
    redraw_window();
}

static void turn_usb_on(uint8_t nu) {
    if (!tud_msc_ejected()) return;
    usb_driver(true);
}

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
    snprintf(line, MAX_WIDTH, "       ");
    // 1, 2, 3... button mark
    line[0] = prec->pre_mark;
    line[1] = prec->mark;
    draw_text(line, left, top, pcs->FOREGROUND_F1_12_COLOR, pcs->BACKGROUND_F1_12_COLOR);
    // button
    snprintf(line, MAX_WIDTH, prec->name);
    draw_text(line, left + 2, top, pcs->FOREGROUND_F_BTN_COLOR, pcs->BACKGROUND_F_BTN_COLOR);
}

static void bottom_line() {
    if (tud_msc_ejected()) {
        if (fn_1_12_tbl_alt[9].action != turn_usb_on) {
            // Alt + F10 no more actions
            sprintf(fn_1_12_tbl_alt[9].name, " USB  ");
            fn_1_12_tbl_alt[9].action = turn_usb_on;
        }
    } else {
        if (fn_1_12_tbl_alt[9].action != turn_usb_off) {
            // Alt + F10 - force unmount usb
            snprintf(fn_1_12_tbl_alt[9].name, BTN_WIDTH, " UnUSB ");
            fn_1_12_tbl_alt[9].action = turn_usb_off;
        }
    }
    int i = 0;
    for (; i < BTNS_COUNT && (i + 1) * BTN_WIDTH < MAX_WIDTH; ++i) {
        const fn_1_12_tbl_rec_t* rec = &(*actual_fn_1_12_tbl())[i];
        draw_fn_btn(rec, i * BTN_WIDTH, F_BTN_Y_POS);
    }
    i = i * BTN_WIDTH;
    for (; i < MAX_WIDTH; ++i) {
        draw_text(" ", i, F_BTN_Y_POS, pcs->FOREGROUND_F1_12_COLOR, pcs->BACKGROUND_F1_12_COLOR);
    }
    draw_cmd_line();
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
    draw_cmd_line();
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

inline static void m_add_file(FILINFO* fi) {
    array_push_back(files_info_arr, new_file_info(fi));
}

static void draw_cmd_line(void) {
    for (size_t i = 0; i < MAX_WIDTH; ++i) {
        draw_text(" ", i, CMD_Y_POS, pcs->FOREGROUND_CMD_COLOR, pcs->BACKGROUND_CMD_COLOR);
    }
    graphics_set_con_pos(0, CMD_Y_POS);
    graphics_set_con_color(13, 0);
    FIL* e = get_stderr(); // TODO: direct draw?
    fprintf(e, "[%s]", get_ctx_var(get_cmd_ctx(), CD));
    graphics_set_con_color(7, 0);
    fprintf(e, "> %s", s_cmd->p);
    graphics_set_con_color(pcs->FOREGROUND_CMD_COLOR, pcs->BACKGROUND_CMD_COLOR);
}

static int m_comp_bna(const void* pe1, const void* pe2) {
    file_info_t* e1 = *((file_info_t**)pe1);
    file_info_t* e2 = *((file_info_t**)pe2);
    if ((e1->fattr & AM_DIR) && !(e2->fattr & AM_DIR)) return -1;
    if (!(e1->fattr & AM_DIR) && (e2->fattr & AM_DIR)) return 1;
    return strcmp(e1->s_name->p, e2->s_name->p);
}

static int m_comp_bnd(const void* pe1, const void* pe2) {
    file_info_t* e1 = *((file_info_t**)pe1);
    file_info_t* e2 = *((file_info_t**)pe2);
    if ((e1->fattr & AM_DIR) && !(e2->fattr & AM_DIR)) return -1;
    if (!(e1->fattr & AM_DIR) && (e2->fattr & AM_DIR)) return 1;
    return strcmp(e2->s_name->p, e1->s_name->p);
}

inline static char* ext(string_t* s) {
    for (int i = s->size - 1; i > 0; --i) {
        if (s->p[i] == '.') {
            return s->p + i + 1;
        }
    }
    return "";
}

static int m_comp_bea(const void* pe1, const void* pe2) {
    file_info_t* e1 = *((file_info_t**)pe1);
    file_info_t* e2 = *((file_info_t**)pe2);
    if ((e1->fattr & AM_DIR) && !(e2->fattr & AM_DIR)) return -1;
    if (!(e1->fattr & AM_DIR) && (e2->fattr & AM_DIR)) return 1;
    return strcmp(ext(e1->s_name), ext(e2->s_name));
}

static int m_comp_bed(const void* pe1, const void* pe2) {
    file_info_t* e1 = *((file_info_t**)pe1);
    file_info_t* e2 = *((file_info_t**)pe2);
    if ((e1->fattr & AM_DIR) && !(e2->fattr & AM_DIR)) return -1;
    if (!(e1->fattr & AM_DIR) && (e2->fattr & AM_DIR)) return 1;
    return strcmp(ext(e2->s_name), ext(e1->s_name));
}

static int m_comp_bsa(const void* pe1, const void* pe2) {
    file_info_t* e1 = *((file_info_t**)pe1);
    file_info_t* e2 = *((file_info_t**)pe2);
    if ((e1->fattr & AM_DIR) && !(e2->fattr & AM_DIR)) return -1;
    if (!(e1->fattr & AM_DIR) && (e2->fattr & AM_DIR)) return 1;
    if ((e1->fattr & AM_DIR) && (e2->fattr & AM_DIR)) {
        return strcmp(e1->s_name->p, e2->s_name->p);
    }
    if (e1->fsize > e2->fsize) return -1;
    if (e1->fsize < e2->fsize) return 1;
    return strcmp(e1->s_name->p, e2->s_name->p);
}

static int m_comp_bsd(const void* pe1, const void* pe2) {
    file_info_t* e1 = *((file_info_t**)pe1);
    file_info_t* e2 = *((file_info_t**)pe2);
    if ((e1->fattr & AM_DIR) && !(e2->fattr & AM_DIR)) return -1;
    if (!(e1->fattr & AM_DIR) && (e2->fattr & AM_DIR)) return 1;
    if ((e1->fattr & AM_DIR) && (e2->fattr & AM_DIR)) {
        return strcmp(e1->s_name->p, e2->s_name->p);
    }
    if (e1->fsize > e2->fsize) return 1;
    if (e1->fsize < e2->fsize) return -1;
    return strcmp(e1->s_name->p, e2->s_name->p);
}

static int m_comp_bda(const void* pe1, const void* pe2) {
    file_info_t* e1 = *((file_info_t**)pe1);
    file_info_t* e2 = *((file_info_t**)pe2);
    if ((e1->fattr & AM_DIR) && !(e2->fattr & AM_DIR)) return -1;
    if (!(e1->fattr & AM_DIR) && (e2->fattr & AM_DIR)) return 1;
    if (e1->fdate > e2->fdate) return 1;
    if (e1->fdate < e2->fdate) return -1;
    if (e1->ftime > e2->ftime) return 1;
    if (e1->ftime < e2->ftime) return -1;
    return strcmp(e1->s_name->p, e2->s_name->p);
}

static int m_comp_bdd(const void* pe1, const void* pe2) {
    file_info_t* e1 = *((file_info_t**)pe1);
    file_info_t* e2 = *((file_info_t**)pe2);
    if ((e1->fattr & AM_DIR) && !(e2->fattr & AM_DIR)) return -1;
    if (!(e1->fattr & AM_DIR) && (e2->fattr & AM_DIR)) return 1;
    if (e2->fdate > e1->fdate) return 1;
    if (e2->fdate < e1->fdate) return -1;
    if (e2->ftime > e1->ftime) return 1;
    if (e2->ftime < e1->ftime) return -1;
    return strcmp(e1->s_name->p, e2->s_name->p);
}

static void collect_files(file_panel_desc_t* p) {
    static __compar_fn_t comp_fn[] = {
        m_comp_bna,
        m_comp_bnd,
        m_comp_bea,
        m_comp_bed,
        m_comp_bsa,
        m_comp_bsd,
        m_comp_bda,
        m_comp_bdd,
    };
    m_cleanup();
    DIR* pdir = (DIR*)malloc(sizeof(DIR));
    if (!m_opendir(pdir, p->s_path->p)) {
        free(pdir);
        return;
    }
    FILINFO* pfileInfo = (FILINFO*)malloc(sizeof(FILINFO));
    while(f_readdir(pdir, pfileInfo) == FR_OK && pfileInfo->fname[0] != '\0') {
        m_add_file(pfileInfo);
    }
    f_closedir(pdir);
    free(pdir);
    if (p->sort_type != BY_NOTHING) {
        qsort(files_info_arr->p, files_info_arr->size, sizeof(void*), comp_fn[p->sort_type]);
    }
}

static void construct_full_name_s(string_t* dst, const string_t* folder, const string_t* file) {
    if (folder && folder->size > 1) {
        string_replace_ss(dst, folder);
    } else {
        string_resize(dst, 0);
    }
    string_push_back_c(dst, '/');
    string_push_back_cs(dst, file);
}

static void construct_full_name(string_t* dst, const char* folder, const char* file) {
    if (folder && strlen(folder) > 1) {
        string_replace_cs(dst, folder);
    } else {
        string_resize(dst, 0);
    }
    string_push_back_c(dst, '/');
    string_push_back_cc(dst, file);
}

static void fill_panel(file_panel_desc_t* p) {
    if (hidePannels) return;
    collect_files(p);
    indexes_t* pp = &p->indexes[p->level];
    if (pp->selected_file_idx < FIRST_FILE_LINE_ON_PANEL_Y) {
        pp->selected_file_idx = FIRST_FILE_LINE_ON_PANEL_Y;
    }
    // TODO: selected should not be out of pannel
//    if (pp->selected_file_idx - pp->start_file_offset > LAST_FILE_LINE_ON_PANEL_Y - FIRST_FILE_LINE_ON_PANEL_Y) {
//        pp->selected_file_idx = pp->start_file_offset + FIRST_FILE_LINE_ON_PANEL_Y; // TODO: test it
//    }
    if (pp->start_file_offset < 0) {
        pp->start_file_offset = 0;
    }
    int y = 1;
    p->files_number = 0;
    int start_file_offset = pp->start_file_offset;
    int selected_file_idx = pp->selected_file_idx;
    int width = p->width;
    if (start_file_offset == 0 && p->s_path->size > 1) {
        draw_label(p->left + 1, y, width - 2, "..", p == psp && selected_file_idx == y, true);
        y++;
        p->files_number++;
    }
    for(int fn = 0; fn < files_info_arr->size; ++ fn) {
        file_info_t* fp = array_get_at(files_info_arr, fn);
        if (start_file_offset <= p->files_number && y <= LAST_FILE_LINE_ON_PANEL_Y) {
            char* filename = fp->s_name->p;
            snprintf(line, MAX_WIDTH >> 1, "%s/%s", p->s_path->p, filename);
            bool selected = p == psp && selected_file_idx == y;
            draw_label(p->left + 1, y, width - 2, filename, selected, fp->fattr & AM_DIR);
            y++;
        }
        p->files_number++;
    }
    for (; y <= LAST_FILE_LINE_ON_PANEL_Y; ++y) {
        draw_label(p->left + 1, y, p->width - 2, "", false, false);
    }
    file_info_t* fp = selected_file(p, false);
    if (fp) {
        // redraw bottom line
        for(int i = 1; i < width - 1; ++i) {
            line[i] = 0xCD; // ═
        }
        line[0]         = 0xC8; // ╚
        line[width - 1] = 0xBC; // ╝
        line[width]     = 0;
        draw_text(line, p->left, PANEL_LAST_Y, pcs->FOREGROUND_FIELD_COLOR, pcs->BACKGROUND_FIELD_COLOR);
        if (fp->fattr & AM_DIR) {
            draw_label(p->left + (width >> 1) - 3, PANEL_LAST_Y, 7, " <DIR> ", false, false);
        } else {
            char t[p->width];
            if (fp->fsize > 100*1024*1024) {
                snprintf(t, width, " %d M ", (uint32_t)(fp->fsize >> 20));
            } else if (fp->fsize > 100*1024) {
                snprintf(t, width, " %d K ", (uint32_t)(fp->fsize >> 10));
            } else {
                snprintf(t, width, " %d B ", (uint32_t)fp->fsize);
            }
            size_t sz = strnlen(t, width);
            draw_label(p->left + (width >> 1) - (sz >> 1), PANEL_LAST_Y, sz, t, false, false);
        }
    }
    if (p == psp) {
        set_ctx_var(get_cmd_ctx(), CD, psp->s_path->p);
        draw_cmd_line();
    }
}

inline static void select_right_panel() {
    psp = right_panel;
    fill_panel(left_panel);
    fill_panel(right_panel);
}

inline static void select_left_panel() {
    psp = left_panel;
    fill_panel(left_panel);
    fill_panel(right_panel);
}

static void m_window() {
    if (hidePannels) return;
    snprintf(line, (MAX_WIDTH >> 1) - 4, "SD:%s", left_panel->s_path->p);
    draw_panel( 0, PANEL_TOP_Y, MAX_WIDTH >> 1, PANEL_LAST_Y + 1, line, 0);
    snprintf(line, (MAX_WIDTH >> 1) - 4, "SD:%s", right_panel->s_path->p);
    draw_panel(MAX_WIDTH >> 1, PANEL_TOP_Y, MAX_WIDTH - (MAX_WIDTH >> 1), PANEL_LAST_Y + 1, line, 0);
}

static scancode_handler_t scancode_handler;

static bool scancode_handler_impl(const uint32_t ps2scancode) { // core ?
    lastCleanableScanCode = ps2scancode & 0xFF; // ensure
    bool numlock = get_leds_stat() & PS2_LED_NUM_LOCK;
    if (ps2scancode == 0xE048 || (ps2scancode == 0x48 && !numlock)) {
        upPressed = true;
        goto r;
    } else if (ps2scancode == 0xE0C8 || (ps2scancode == 0xC8 && !numlock)) {
        upPressed = false;
        goto r;
    } else if (ps2scancode == 0xE050 || (ps2scancode == 0x50 && !numlock)) {
        downPressed = true;
        goto r;
    } else if (ps2scancode == 0xE0D0 || (ps2scancode == 0xD0 && !numlock)) {
        downPressed = false;
        goto r;
    }
    register uint8_t sc = (uint8_t)ps2scancode & 0xFF;
    if (sc == 0x4B) {
        leftPressed = true;
    } else if (sc == 0xCB) {
        leftPressed = false;
    } else if (sc == 0x4D) {
        rightPressed = true;
    } else if (sc == 0xCD) {
        rightPressed = false;
    } else if (sc == 0x38) {
        altPressed = true;
    } else if (sc == 0xB8) {
        altPressed = false;
    } else if (sc == 0x1D) {
        ctrlPressed = true;
    } else if (sc == 0x9D) {
        ctrlPressed = false;
    }
r:
    if (scancode_handler) {
        return scancode_handler(ps2scancode);
    }
    return false;
}

static inline void redraw_current_panel() {
    if (!hidePannels) {
        snprintf(line, (MAX_WIDTH >> 1) - 4, "SD:%s", psp->s_path->p);
        draw_panel(psp->left, PANEL_TOP_Y, psp->width, PANEL_LAST_Y + 1, line, 0);
        fill_panel(psp);
        set_ctx_var(get_cmd_ctx(), CD, psp->s_path->p);
    }
    draw_cmd_line();
}

inline static cmd_ctx_t* new_ctx(cmd_ctx_t* src) {
    cmd_ctx_t* res = (cmd_ctx_t*)pvPortMalloc(sizeof(cmd_ctx_t));
    memset(res, 0, sizeof(cmd_ctx_t));
    if (src->vars_num && src->vars) {
        res->vars = (vars_t*)pvPortMalloc( sizeof(vars_t) * src->vars_num );
        res->vars_num = src->vars_num;
        for (size_t i = 0; i < src->vars_num; ++i) {
            if (src->vars[i].value) {
                res->vars[i].value = copy_str(src->vars[i].value);
            }
            res->vars[i].key = src->vars[i].key; // const
        }
    }
    res->stage = src->stage;
    return res;
}

inline static char replace_spaces0(char t) {
    return (t == ' ') ? 0 : t;
}

inline static size_t in_quotas(size_t i, string_t* pcmd, string_t* t) {
    for (; i < pcmd->size; ++i) {
        char c = pcmd->p[i];
        if (c == '"') {
            return i;
        }
        string_push_back_c(t, c);
    }
    return i;
}

inline static void tokenize_cmd(list_t* lst, string_t* pcmd, cmd_ctx_t* ctx) {
    #ifdef DEBUG
        printf("[tokenize_cmd] %s\n", pcmd->p);
    #endif
    while (pcmd->size && pcmd->p[0] == ' ') { // remove trailing spaces
        string_clip(pcmd, 0);
        #ifdef DEBUG
            printf("[tokenize_cmd] clip: %s\n", pcmd->p);
        #endif
    }
    if (!pcmd->size) {
        return 0;
    }
    bool in_space = false;
    int inTokenN = 0;
    string_t* t = new_string_v();
    for (size_t i = 0; i < pcmd->size; ++i) {
        char c = pcmd->p[i];
        if (c == '"') {
            if(t->size) {
                list_push_back(lst, t);
                t = new_string_v();
            }
            i = in_quotas(++i, pcmd, t);
            in_space = false;
            continue;
        }
        c = replace_spaces0(c);
        if (in_space) {
            if(c) { // token started
                in_space = false;
                list_push_back(lst, t);
                t = new_string_v();
            }
        } else if(!c) { // not in space, after the token
            in_space = true;
        }
        string_push_back_c(t, c);
    }
    if (t->size) list_push_back(lst, t);
    #ifdef DEBUG
        node_t* n = lst->first;
        while(n) {
            printf("[tokenize_cmd] n: %s\n", c_str(n->data));
            n = n->next;
        }
    #endif
}

inline static string_t* get_std_out1(bool* p_append, string_t* pcmd, size_t i0, size_t sz) {
    #ifdef DEBUG
        printf("[get_std_out1] [%s] i0: %d\n", pcmd->p + i0, i0);
    #endif
    string_t* std_out_file = new_string_v();
    for (size_t i = i0; i < sz; ++i) {
        char c = pcmd->p[i];
        #ifdef DEBUG
            printf("[get_std_out1] [%s] c: %c\n", pcmd->p + i, c);
        #endif
        if (i == i0 && c == '>') {
            *p_append = true;
            continue;
        }
        if (c == '"') {
            in_quotas(++i, pcmd, std_out_file);
            break;
        }
        if (c != ' ') {
            string_push_back_c(std_out_file, c);
        }
    }
    return std_out_file;
}

inline static string_t* get_std_out0(bool* p_append, string_t* pcmd) {
    #ifdef DEBUG
        printf("[get_std_out0] %s\n", pcmd->p);
    #endif
    bool in_quotas = false;
    size_t sz = pcmd->size;
    for (size_t i = 0; i < sz; ++i) {
        char c = pcmd->p[i];
        if (c == '"') {
            in_quotas = !in_quotas;
        }
        if (!in_quotas && c == '>') {
            string_t* t = get_std_out1(p_append, pcmd, i + 1, sz);
            string_resize(pcmd, i); // use only left side of the string
            return t;
        }
    }
    return NULL;
}

inline static bool prepare_ctx(string_t* pcmd, cmd_ctx_t* ctx) {
    #ifdef DEBUG
        printf("[prepare_ctx] %s\n", pcmd->p);
    #endif
    bool in_quotas = false;
    bool append = false;
    string_t* std_out_file = get_std_out0(&append, pcmd);
    if (std_out_file) {
        #ifdef DEBUG
            printf("[prepare_ctx] stdout: '%s'; append: %d\n", std_out_file->p, append);
        #endif
        ctx->std_out = calloc(1, sizeof(FIL));
        if (FR_OK != f_open(ctx->std_out, std_out_file->p, FA_WRITE | (append ? FA_OPEN_APPEND : FA_CREATE_ALWAYS))) {
            printf("Unable to open file: '%s'\n", std_out_file->p);
            delete_string(std_out_file);
            return false;
        }
        delete_string(std_out_file);
    }

    list_t* lst = new_list_v(new_string_v, delete_string, NULL);
    tokenize_cmd(lst, pcmd, ctx);
    if (!lst->size) {
        delete_list(lst);
        return false;
    }

    ctx->argc = lst->size;
    ctx->argv = (char**)calloc(sizeof(char*), lst->size);
    node_t* n = lst->first;
    for(size_t i = 0; i < lst->size && n != NULL; ++i, n = n->next) {
        ctx->argv[i] = copy_str(c_str(n->data));
    }
    delete_list(lst);
    if (ctx->orig_cmd) free(ctx->orig_cmd);
    ctx->orig_cmd = copy_str(ctx->argv[0]);
    ctx->stage = PREPARED;
    return true;
}

inline static void cmd_write_history(cmd_ctx_t* ctx) {
    char* tmp = get_ctx_var(ctx, TEMP);
    if(!tmp) tmp = "";
    size_t cdl = strlen(tmp);
    char * cmd_history_file = concat(tmp, _cmd_history);
    FIL* pfh = (FIL*)malloc(sizeof(FIL));
    f_open(pfh, cmd_history_file, FA_OPEN_ALWAYS | FA_WRITE | FA_OPEN_APPEND);
    UINT br;
    f_write(pfh, s_cmd->p, s_cmd->size, &br);
    f_write(pfh, "\n", 1, &br);
    f_close(pfh);
    free(pfh);
    free(cmd_history_file);
}

static bool cmd_enter(cmd_ctx_t* ctx) {
    putc('\n');
    if (s_cmd->size) {
        cmd_write_history(ctx);
    } else {
        goto r2;
    }
    #ifdef DEBUG
        printf("[cmd_enter] %s\n", s_cmd->p);
    #endif
    bool exit = false;
    bool in_qutas = false;
    cmd_ctx_t* ctxi = ctx;
    string_t* t = new_string_v();
    for (size_t i = 0; i <= s_cmd->size; ++i) {
        char c = s_cmd->p[i];
        if (!c) {
            #ifdef DEBUG
                printf("[cmd_enter] [%s] pass to [prepare_ctx]\n", t->p);
            #endif
            exit = prepare_ctx(t, ctxi);
            break;
        }
        if (c == '"') {
            in_qutas = !in_qutas;
        }
        if (!in_qutas && c == '|') {
            cmd_ctx_t* curr = ctxi;
            cmd_ctx_t* next = new_ctx(ctxi);
            exit = prepare_ctx(t, curr);
            curr->std_out = calloc(1, sizeof(FIL));
            curr->std_err = curr->std_out;
            next->std_in = calloc(1, sizeof(FIL));
            f_open_pipe(curr->std_out, next->std_in);
            curr->detached = true;
            next->prev = curr;
            curr->next = next;
            next->detached = false;
            ctxi = next;
            string_resize(t, 0);
            continue;
        }
        if (!in_qutas && c == '&') {
            exit = prepare_ctx(t, ctxi);
            ctxi->detached = true;
            string_resize(t, 0);
            break;
        }
        string_push_back_c(t, c);
    }
    delete_string(t);
    if (exit) { // prepared ctx
        return true;
    }
    ctxi = ctx->next;
    ctx->next = 0;
    while(ctxi) { // remove pipe chain
        cmd_ctx_t* next = ctxi->next;
        remove_ctx(ctxi);
        ctxi = next;
    }
    cleanup_ctx(ctx); // base ctx to be there
r2:
    string_resize(s_cmd, 0);
    draw_cmd_line();
    return false;
}

static void enter_pressed() {
    if (s_cmd->size && !ctrlPressed) {
        mark_to_exit_flag = cmd_enter(get_cmd_ctx());
        return;
    }
    if (hidePannels) {
        return;
    }
    file_info_t* fp = selected_file(psp, true);
    if (!fp) { // up to parent dir
        int i = psp->s_path->size;
        while(--i > 0) {
            char c = psp->s_path->p[i];
            if (c == '\\' || c == '/') {
                string_resize(psp->s_path, i);
                psp->level--;
                redraw_current_panel();
                return;
            }
        }
        string_replace_cs(psp->s_path, "/");
#if EXT_DRIVES_MOUNT
        psp->in_dos = false;
#endif
        psp->level--;
        redraw_current_panel();
        return;
    }
    if (fp->fattr & AM_DIR) {
        string_t* path = new_string_v();
        construct_full_name_s(path, psp->s_path, fp->s_name);
        string_replace_ss(psp->s_path, path);
        delete_string(path);
        psp->level++;
        if (psp->level >= 16) {
            psp->level = 15;
        }
        psp->indexes[psp->level].selected_file_idx = FIRST_FILE_LINE_ON_PANEL_Y;
        psp->indexes[psp->level].start_file_offset = 0;
        redraw_current_panel();
        return;
    }
    if (ctrlPressed) {
        if (psp->s_path->size > 1)
            string_push_back_cs(s_cmd, psp->s_path); // quotas?
        string_push_back_c(s_cmd, '/');
        string_push_back_cs(s_cmd, fp->s_name);
        draw_cmd_line();
        return;
    }
    construct_full_name_s(s_cmd, psp->s_path, fp->s_name);
    gouta(s_cmd->p);
    mark_to_exit_flag = cmd_enter(get_cmd_ctx());
}

inline static void handle_pagedown_pressed() {
    indexes_t* p = &psp->indexes[psp->level];
    for (int i = 0; i < MAX_HEIGHT / 2; ++i) {
        if (p->selected_file_idx < LAST_FILE_LINE_ON_PANEL_Y &&
            p->start_file_offset + p->selected_file_idx < psp->files_number
        ) {
            p->selected_file_idx++;
        } else if (
            p->selected_file_idx == LAST_FILE_LINE_ON_PANEL_Y &&
            p->start_file_offset + p->selected_file_idx < psp->files_number
        ) {
            p->selected_file_idx--;
            p->start_file_offset++;
        }
    }
    fill_panel(psp);
    scan_code_processed();
}

static int cmd_history_idx = -2;

inline static int history_steps(cmd_ctx_t* ctx) {
    char* tmp = get_ctx_var(ctx, TEMP);
    if(!tmp) tmp = "";
    size_t cdl = strlen(tmp);
    char * cmd_history_file = concat(tmp, _cmd_history);
    FIL* pfh = (FIL*)malloc(sizeof(FIL));
    int idx = 0;
    UINT br;
    f_open(pfh, cmd_history_file, FA_READ);
    char* b = malloc(512);
    while(f_read(pfh, b, 512, &br) == FR_OK && br) {
        for(size_t i = 0; i < br; ++i) {
            char t = b[i];
            if(t == '\n') { // next line
                if(cmd_history_idx == idx)
                    break;
                string_resize(s_cmd, 0);
                idx++;
            } else {
                string_push_back_c(s_cmd, t);
            }
        }
    }
    free(b);
    f_close(pfh);
    free(pfh);
    free(cmd_history_file);
    return idx;
}

inline static void cancel_entered() {
    while(s_cmd->size) {
        string_resize(s_cmd, s_cmd->size - 1);
        gbackspace();
    }
}

inline static void cmd_up(cmd_ctx_t* ctx) {
    cancel_entered();
    cmd_history_idx--;
    int idx = history_steps(ctx);
    if (cmd_history_idx < 0) cmd_history_idx = idx;
    gouta(s_cmd->p);
}

inline static void cmd_down(cmd_ctx_t* ctx) {
    cancel_entered();
    if (cmd_history_idx == -2) cmd_history_idx = -1;
    cmd_history_idx++;
    history_steps(ctx);
    gouta(s_cmd->p);
}

inline static void handle_down_pressed() {
    if (hidePannels) {
        cmd_down(get_cmd_ctx());
        return;
    }
    indexes_t* p = &psp->indexes[psp->level];
    if (p->selected_file_idx < LAST_FILE_LINE_ON_PANEL_Y &&
        p->start_file_offset + p->selected_file_idx < psp->files_number
    ) {
        p->selected_file_idx++;
        fill_panel(psp);
    } else if (
        p->selected_file_idx == LAST_FILE_LINE_ON_PANEL_Y &&
        p->start_file_offset + p->selected_file_idx < psp->files_number
    ) {
        p->selected_file_idx -= 5;
        p->start_file_offset += 5;
        fill_panel(psp);    
    }
    scan_code_processed();
}

inline static void handle_pageup_pressed() {
    indexes_t* p = &psp->indexes[psp->level];
    for (int i = 0; i < MAX_HEIGHT / 2; ++i) {
        if (p->selected_file_idx > FIRST_FILE_LINE_ON_PANEL_Y) {
            p->selected_file_idx--;
        } else if (p->selected_file_idx == FIRST_FILE_LINE_ON_PANEL_Y && p->start_file_offset > 0) {
            p->selected_file_idx++;
            p->start_file_offset--;
        }
    }
    fill_panel(psp);
    scan_code_processed();
}

inline static void handle_up_pressed() {
    if (hidePannels) {
        cmd_up(get_cmd_ctx());
        return;
    }
    indexes_t* p = &psp->indexes[psp->level];
    if (p->selected_file_idx > FIRST_FILE_LINE_ON_PANEL_Y) {
        p->selected_file_idx--;
        fill_panel(psp);
    } else if (p->selected_file_idx == FIRST_FILE_LINE_ON_PANEL_Y && p->start_file_offset > 0) {
        p->selected_file_idx += 5;
        p->start_file_offset -= 5;
        fill_panel(psp);       
    }
    scan_code_processed();
}

inline static fn_1_12_btn_pressed(uint8_t fn_idx) {
    if (fn_idx > 11) fn_idx -= 18; // F11-12
    (*actual_fn_1_12_tbl())[fn_idx].action(fn_idx);
}

inline static void cmd_backspace() {
    if (s_cmd->size == 0) {
        blimp(10, 5);
        return;
    }
    string_resize(s_cmd, s_cmd->size - 1);
    gbackspace();
}

inline static char* next_on(char* l, char *bi, bool in_quotas) {
    char *b = bi;
    while(*l && *b && *l == *b) {
        if (*b == ' ' && !in_quotas) break;
        l++;
        b++;
    }
    if (*l == 0 && !in_quotas) {
        char* bb = b;
        while(*bb) {
            if (*bb == ' ') {
                return bi;
            }
            bb++;
        }
    }
    return *l == 0 ? b : bi;
}

inline static void type_char(char c) {
    putc(c);
    string_push_back_c(s_cmd, c);
}

inline static void cmd_tab(cmd_ctx_t* ctx) {
    char * p = s_cmd->p;
    char * p2 = p;
    bool in_quotas = false;
    while (*p) {
        char c = *p++;
        if (c == '"') {
            p2 = p;
            in_quotas = true;
            break;
        }
        if (c == ' ') {
            p2 = p;
        }
    }
    p = p2;
    char * p3 = p2;
    while (*p3) {
        if (*p3++ == '/') {
            p2 = p3;
        }
    }
    string_t* s_b = NULL;
    if (p != p2) {
        s_b = new_string_cc(p);
        string_resize(s_b, p2 - p);
    } else {
        s_b = new_string_v();
    }
    DIR* pdir = (DIR*)malloc(sizeof(DIR));
    FILINFO* pfileInfo = (FILINFO*)malloc(sizeof(FILINFO));
    //goutf("\nDIR: %s\n", p != p2 ? b : curr_dir);
    if (FR_OK != f_opendir(pdir, p != p2 ? s_b->p : get_ctx_var(ctx, CD))) {
        delete_string(s_b);
        return;
    }
    int total_files = 0;
    while (f_readdir(pdir, pfileInfo) == FR_OK && pfileInfo->fname[0] != '\0') {
        p3 = next_on(p2, pfileInfo->fname, in_quotas);
        if (p3 != pfileInfo->fname) {
            string_replace_cs(s_b, p3);
            total_files++;
            break; // TODO: variants
        }
        //goutf("p3: %s; p2: %s; fn: %s; cmd_t: %s; fls: %d\n", p3, p2, fileInfo.fname, b, total_files);
    }
    if (total_files == 1) {
        p3 = s_b->p;
        while (*p3) {
            type_char(*p3++);
        }
        if (in_quotas) {
            type_char('"');
        }
    } else {
        blimp(10, 5);
    }
    delete_string(s_b);
    f_closedir(pdir);
    free(pfileInfo);
    free(pdir);
}

inline static void handle_tab_pressed() {
    if (hidePannels) {
        cmd_tab(get_cmd_ctx());
        return;
    }
    if (psp == left_panel) {
        select_right_panel();
        return;
    }
    select_left_panel();
}

inline static void restore_console(cmd_ctx_t* ctx) {
    char* tmp = get_ctx_var(ctx, TEMP);
    if(!tmp) tmp = "";
    size_t cdl = strlen(tmp);
    char * mc_con_file = concat(tmp, _mc_con);
    FIL* pfh = (FIL*)malloc(sizeof(FIL));
    if (FR_OK != f_open(pfh, mc_con_file, FA_READ)) {
        goto r;
    }
    char* b = get_buffer();
    uint32_t w = get_screen_width();
    uint32_t h = get_screen_height();
    uint8_t bit = get_screen_bitness();
    size_t sz = (w * h * bit) >> 3;
    UINT rb;
    f_read(pfh, b, sz, &rb);
    f_close(pfh);
r:
    free(pfh);
    free(mc_con_file);
}

inline static void save_console(cmd_ctx_t* ctx) {
    char* tmp = get_ctx_var(ctx, TEMP);
    if(!tmp) tmp = "";
    size_t cdl = strlen(tmp);
    char * mc_con_file = concat(tmp, _mc_con);
    FIL* pfh = (FIL*)malloc(sizeof(FIL));
    if (FR_OK != f_open(pfh, mc_con_file, FA_CREATE_ALWAYS | FA_WRITE)) {
        goto r;
    }
    char* b = get_buffer();
    uint32_t w = get_screen_width();
    uint32_t h = get_screen_height();
    uint8_t bit = get_screen_bitness();
    size_t sz = (w * h * bit) >> 3;
    UINT wb;
    f_write(pfh, b, sz, &wb);
    f_close(pfh);
r:
    free(pfh);
    free(mc_con_file);
}

inline static void hide_pannels() {
    hidePannels = !hidePannels;
    if (hidePannels) {
        restore_console(get_cmd_ctx());
        bottom_line();
    } else {
        save_console(get_cmd_ctx());
        m_window();
        fill_panel(left_panel);
        fill_panel(right_panel);
    }
}

inline static void save_rc() {
    cmd_ctx_t* ctx = get_cmd_ctx();
    char* tmp = get_ctx_var(ctx, TEMP);
    if(!tmp) tmp = "";
    size_t cdl = strlen(tmp);
    char * mc_rc_file = concat(tmp, _mc_res);
    FIL* pfh = (FIL*)malloc(sizeof(FIL));
    if (FR_OK != f_open(pfh, mc_rc_file, FA_CREATE_ALWAYS | FA_WRITE)) {
        goto r;
    }
    UINT wb;
    size_t sz0 = left_panel->s_path->size + 1;
    f_write(pfh, left_panel, sizeof(file_panel_desc_t), &wb);
    f_write(pfh, &sz0, sizeof(size_t), &wb);
    f_write(pfh, left_panel->s_path->p, sz0, &wb);
    f_write(pfh, right_panel, sizeof(file_panel_desc_t), &wb);
    sz0 = right_panel->s_path->size + 1;
    f_write(pfh, &sz0, sizeof(size_t), &wb);
    f_write(pfh, right_panel->s_path->p, sz0, &wb);
    bool left_selected = (psp == left_panel);
    f_write(pfh, &left_selected, sizeof(left_selected), &wb);
    f_write(pfh, &hidePannels, sizeof(hidePannels), &wb);
    f_close(pfh);
r:
    free(pfh);
    free(mc_rc_file);
}

inline static bool initi_from_rc(cmd_ctx_t* ctx) {
    bool res = false;
    char* tmp = get_ctx_var(ctx, TEMP);
    if(!tmp) tmp = "";
    size_t cdl = strlen(tmp);
    char * mc_rc_file = concat(tmp, _mc_res);
    FIL* pfh = (FIL*)malloc(sizeof(FIL));
    if (FR_OK != f_open(pfh, mc_rc_file, FA_READ)) {
        goto r1;
    }
    UINT rb;
    string_t* preserv_string = left_panel->s_path;
    if(FR_OK != f_read(pfh, left_panel, sizeof(file_panel_desc_t), &rb)) {
        goto r2;
    }
    size_t panel_path_sz;

    f_read(pfh, &panel_path_sz, sizeof(size_t), &rb);
    left_panel->s_path = preserv_string;
    string_reseve(left_panel->s_path, panel_path_sz);
    f_read(pfh, left_panel->s_path->p, panel_path_sz, &rb);
    left_panel->s_path->size = panel_path_sz - 1;

    preserv_string = right_panel->s_path;
    if(FR_OK != f_read(pfh, right_panel, sizeof(file_panel_desc_t), &rb)) {
        goto r2;
    }

    f_read(pfh, &panel_path_sz, sizeof(size_t), &rb);
    right_panel->s_path = preserv_string;
    string_reseve(right_panel->s_path, panel_path_sz);
    f_read(pfh, right_panel->s_path->p, panel_path_sz, &rb);
    right_panel->s_path->size = panel_path_sz - 1;

    bool left_selected;
    if(FR_OK != f_read(pfh, &left_selected, sizeof(left_selected), &rb)) {
        goto r2;
    }
    if (!left_selected) {
        psp = right_panel;
    }
    if(FR_OK != f_read(pfh, &hidePannels, sizeof(hidePannels), &rb)) {
        goto r2;
    }
    res = true;
r2:
    f_close(pfh);
r1:
    free(pfh);
r:
    free(mc_rc_file);
    return res;
}

inline static void esc_pressed(void) {
    blimp(15, 1);
    string_resize(s_cmd, 0);
}

static inline void work_cycle(cmd_ctx_t* ctx) {
    uint8_t repeat_cnt = 0;
    for(;;) {
        char c = getch_now();
        nespad_stat(&nespad_state, &nespad_state2);
        if (c) {
            if (c == CHAR_CODE_BS) cmd_backspace();
            else if (c == CHAR_CODE_UP) handle_up_pressed();
            else if (c == CHAR_CODE_DOWN) handle_down_pressed();
            else if (c == CHAR_CODE_TAB) handle_tab_pressed();
            else if (c == CHAR_CODE_ENTER) enter_pressed();
            else if (c == CHAR_CODE_ESC) esc_pressed();
            else if (ctrlPressed && (c == 'o' || c == 'O' || c == 0x99 /*Щ*/ || c == 0xE9 /*щ*/)) hide_pannels();
            else type_char(c);
        }

        if (is_dendy_joystick) {
            if (is_dendy_joystick) nespad_read();
            if (nespad_state_delay > 0) {
                nespad_state_delay--;
                sleep_ms(1);
            }
            else {
                nespad_state_delay = DPAD_STATE_DELAY;
                if(nespad_state & DPAD_UP) {
                    handle_up_pressed();
                } else if(nespad_state & DPAD_DOWN) {
                    handle_down_pressed();
                } else if (nespad_state & DPAD_B) {
                    if (nespad_state & DPAD_SELECT) {
                        turn_usb_on(0);
                    }
                } else if (nespad_state & DPAD_A) {
                    ctrlPressed = nespad_state & DPAD_SELECT;
                    enter_pressed();
                } else if ((nespad_state & DPAD_LEFT) || (nespad_state & DPAD_RIGHT)) {
                    handle_tab_pressed();
                }
            }
        }
        if (lastSavedScanCode != lastCleanableScanCode && lastSavedScanCode > 0x80) {
            repeat_cnt = 0;
        } else {
            repeat_cnt++;
            if (repeat_cnt > 0xFE && lastSavedScanCode < 0x80) {
               lastCleanableScanCode = lastSavedScanCode + 0x80;
               repeat_cnt = 0;
            }
        }
        register sc = lastCleanableScanCode;
        if (sc == 0x01 || sc == 0x81) { // Esc
            // ?
        } else if ((sc >= 0x3B && sc <= 0x44) || sc == 0x57 || sc == 0x58 || sc == 0x49 || sc == 0x51 || sc == 0x1C || sc == 0x9C) { // F1..12 down,/..
            scan_code_processed();
        } else if ((sc >= 0xBB && sc <= 0xC4) || sc == 0xD7 || sc == 0xD8) { // F1..12 up
            if (lastSavedScanCode == sc - 0x80) {
                fn_1_12_btn_pressed(sc - 0xBB);
                scan_code_processed();
            }
        } else if (sc == 0x1D || sc == 0x9D || sc == 0x38 || sc == 0xB8) { // Ctrl / Alt
            bottom_line();
            scan_code_processed();
        } else if (sc == 0xC9) {
            if (lastSavedScanCode == 0x49) {
                handle_pageup_pressed();
            }
        } else if (sc == 0xD1) {
            if (lastSavedScanCode == 0x51) {
                handle_pagedown_pressed();
            }
        }
        // TODO:
        //  case 0xCB: // left
        //  case 0xCD: // right
        if(mark_to_exit_flag) {
            save_rc();
            restore_console(ctx);
            show_logo(false);
            draw_cmd_line();
            FIL* e = get_stderr(); // TODO: direct draw?
            fprintf(e, "\n");
            return;
        }
    }
}

inline static void start_manager(cmd_ctx_t* ctx) {
    m_window();
    fill_panel(left_panel);
    fill_panel(right_panel);
    update_menu_color();
    work_cycle(ctx);
}

int main(void) {
    cmd_ctx_t* ctx = get_cmd_ctx();
//    if (ctx->argc == 0) {
//        fprintf(ctx->std_err, "ATTTENTION! BROKEN EXECUTION CONTEXT [%p]!\n", ctx);
//        return -1;
//    }
    MAX_WIDTH = get_console_width();
    MAX_HEIGHT = get_console_height();
    F_BTN_Y_POS = TOTAL_SCREEN_LINES - 1;
    CMD_Y_POS = F_BTN_Y_POS - 1;
    PANEL_LAST_Y = CMD_Y_POS - 1;
    line = calloc(MAX_WIDTH + 2, 1);
    LAST_FILE_LINE_ON_PANEL_Y = PANEL_LAST_Y - 1;
    save_console(ctx);

    s_cmd = new_string_v();

    left_panel = calloc(1, sizeof(file_panel_desc_t));
    left_panel->sort_type = BY_NAME_ASC;
    left_panel->s_path = new_string_cc("/");
    right_panel = calloc(1, sizeof(file_panel_desc_t));
    right_panel->sort_type = BY_NAME_ASC;
    right_panel->s_path = new_string_cc("/");
    psp = left_panel;

    if (!initi_from_rc(ctx)) {
        left_panel->indexes[0].selected_file_idx = FIRST_FILE_LINE_ON_PANEL_Y;
        right_panel->indexes[0].selected_file_idx = FIRST_FILE_LINE_ON_PANEL_Y;
    }
    // TODO: in case other mode, dynamic size, etc...
    left_panel->width = MAX_WIDTH >> 1;
    right_panel->left = MAX_WIDTH >> 1;
    right_panel->width = MAX_WIDTH - right_panel->left;

    pcs = calloc(1, sizeof(color_schema_t));
    pcs->BACKGROUND_FIELD_COLOR = 1, // Blue
    pcs->FOREGROUND_FIELD_COLOR = 7, // White
    pcs->HIGHLIGHTED_FIELD_COLOR = 15, // LightWhite
    pcs->BACKGROUND_F1_12_COLOR = 0, // Black
    pcs->FOREGROUND_F1_12_COLOR = 7, // White
    pcs->BACKGROUND_F_BTN_COLOR = 3, // Green
    pcs->FOREGROUND_F_BTN_COLOR = 0, // Black
    pcs->BACKGROUND_CMD_COLOR = 0, // Black
    pcs->FOREGROUND_CMD_COLOR = 7, // White
    pcs->BACKGROUND_SEL_BTN_COLOR = 11, // Light Blue
    pcs->FOREGROUND_SELECTED_COLOR = 0, // Black
    pcs->BACKGROUND_SELECTED_COLOR = 11, // Light Blue

    files_info_arr = new_array_v(fi_allocator, fi_deallocator, NULL);
    scancode_handler = get_scancode_handler();
    set_scancode_handler(scancode_handler_impl);

    start_manager(ctx);

    set_scancode_handler(scancode_handler);
    delete_array(files_info_arr);
    free(line);
    delete_string(right_panel->s_path);
    free(right_panel);
    delete_string(left_panel->s_path);
    free(left_panel);
    free(pcs);
    delete_string(s_cmd);

    return 0;
}

int __required_m_api_verion(void) {
    return M_API_VERSION;
}
