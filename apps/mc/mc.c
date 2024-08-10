#include "m-os-api.h"

static void m_window();
static void redraw_window();
static void draw_cmd_line(int left, int top);
static void bottom_line();
static void construct_full_name(char* dst, const char* folder, const char* file);
static bool m_prompt(const char* txt);
static void no_selected_file();
static bool cmd_enter(cmd_ctx_t* ctx, const char* cmd);
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
static volatile bool aPressed = false;
static volatile bool cPressed = false;
static volatile bool gPressed = false;
static volatile bool tPressed = false;
static volatile bool dPressed = false;
static volatile bool sPressed = false;
static volatile bool xPressed = false;
static volatile bool ePressed = false;
static volatile bool uPressed = false;
static volatile bool hPressed = false;

// TODO:
static bool is_dendy_joystick = true;
static bool is_kbd_joystick = false;
#define DPAD_STATE_DELAY 200
static int nespad_state_delay = DPAD_STATE_DELAY;
static uint8_t nespad_state, nespad_state2;
static bool mark_to_exit_flag = false;

static char* cmd = 0;

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
    int dir_num;
} indexes_t;

typedef struct file_panel_desc {
    int left;
    int width;
    int files_number;
    string_t* s_path;
    indexes_t indexes[16]; // TODO: some ext. limit logic
    int level;
#if EXT_DRIVES_MOUNT
    bool in_dos;
#endif
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
    FSIZE_t fsize;   /* File size */
    WORD    fdate;   /* Modified date */
    WORD    ftime;   /* Modified time */
    BYTE    fattrib; /* File attribute */
    string_t* s_name; //[MAX_WIDTH >> 1];
    int     dir_num;
} file_info_t;
static file_info_t* selected_file();

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
    hidePannels = false;

    backspacePressed = false;
    enterPressed = false;
    plusPressed = false;
    minusPressed = false;
    ctrlPressed = false;
    altPressed = false;
    delPressed = false;
    f1Pressed = false;
    f2Pressed = false;
    f3Pressed = false;
    f4Pressed = false;
    f5Pressed = false;
    f6Pressed = false;
    f7Pressed = false;
    f8Pressed = false;
    f9Pressed = false;
    f10Pressed = false;
    f11Pressed = false;
    f12Pressed = false;
    tabPressed = false;
    spacePressed = false;
    escPressed = false;
    leftPressed = false;
    rightPressed = false;
    upPressed = false;
    downPressed = false;
    aPressed = false;
    cPressed = false;
    gPressed = false;
    tPressed = false;
    dPressed = false;
    sPressed = false;
    xPressed = false;
    ePressed = false;
    uPressed = false;
    hPressed = false;

    is_dendy_joystick = true;
    is_kbd_joystick = false;
    nespad_state_delay = DPAD_STATE_DELAY;
    nespad_state = nespad_state2 = 0;
    mark_to_exit_flag = false;
    scan_code_cleanup();
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

static void reset(uint8_t cmd) {
    f12Pressed = false;
    // TODO:
}

static file_info_t* selected_file() {
    int start_file_offset = psp->indexes[psp->level].start_file_offset;
    int selected_file_idx = psp->indexes[psp->level].selected_file_idx;
    if (selected_file_idx == FIRST_FILE_LINE_ON_PANEL_Y && start_file_offset == 0 && psp->s_path->size > 1) {
        return 0;
    }
    collect_files(psp);
    int y = 1;
    int files_number = 0;
    if (start_file_offset == 0 && psp->s_path->size > 1) {
        y++;
        files_number++;
    }
    for(int fn = 0; fn < files_count; ++ fn) {
        file_info_t* fp = &files_info[fn];
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
    DIR dir;
    FRESULT res = f_opendir(&dir, path);
    if (res != FR_OK) return res;
    FILINFO fileInfo;
    while(f_readdir(&dir, &fileInfo) == FR_OK && fileInfo.fname[0] != '\0') {
        char path2[256];
        construct_full_name(path2, path, fileInfo.fname);
        if (fileInfo.fattrib & AM_DIR) {
            res = m_unlink_recursive(path2);
        } else {
            res = f_unlink(path2);
        }
        if (res != FR_OK) break;
    }
    f_closedir(&dir);
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
//    gpio_put(PICO_DEFAULT_LED_PIN, true);
    file_info_t* fp = selected_file();
    if (!fp) {
       no_selected_file();
       return;
    }
    char path[256];
    snprintf(path, 256, "Remove %s %s?", fp->s_name->p, fp->fattrib & AM_DIR ? "folder" : "file");
    if (m_prompt(path)) {
        construct_full_name(path, psp->s_path->p, fp->s_name->p);
        FRESULT result = fp->fattrib & AM_DIR ? m_unlink_recursive(path) : f_unlink(path);
        if (result != FR_OK) {
            snprintf(line, MAX_WIDTH, "FRESULT: %d", result);
            const line_t lns[3] = {
                { -1, "Unable to delete selected item!" },
                { -1, path },
                { -1, line }
            };
            const lines_t lines = { 3, 2, lns };
            draw_box((MAX_WIDTH - 60) / 2, 7, 60, 10, "Error", &lines);
            sleep_ms(2500);
        } else {
            psp->indexes[psp->level].selected_file_idx--;
        }
    }
//    gpio_put(PICO_DEFAULT_LED_PIN, false);
    redraw_window();    
}

inline static FRESULT m_copy(char* path, char* dest) {
    FIL file1;
    FRESULT result = f_open(&file1, path, FA_READ);
    if (result != FR_OK) return result;
    FIL file2;
    result = f_open(&file2, dest, FA_WRITE | FA_CREATE_ALWAYS);
    if (result != FR_OK) {
        f_close(&file1);
        return result;
    }
    UINT br;
    do {
        result = f_read(&file1, files_info, sizeof(files_info), &br);
        if (result != FR_OK || br == 0) break;
        UINT bw;
        f_write(&file2, files_info, br, &bw);
        if (result != FR_OK) break;
    } while (br);
    f_close(&file1);
    f_close(&file2);
    return result;
}

inline static FRESULT m_copy_recursive(char* path, char* dest) {
 //   gpio_put(PICO_DEFAULT_LED_PIN, true);
    DIR dir;
    FRESULT res = f_opendir(&dir, path);
    if (res != FR_OK) return res;
    res = f_mkdir(dest);
    if (res != FR_OK) return res;
    FILINFO fileInfo;
    while(f_readdir(&dir, &fileInfo) == FR_OK && fileInfo.fname[0] != '\0') {
        char path2[256];
        construct_full_name(path2, path, fileInfo.fname);
        char dest2[256];
        construct_full_name(dest2, dest, fileInfo.fname);
        if (fileInfo.fattrib & AM_DIR) {
            res = m_copy_recursive(path2, dest2);
        } else {
            res = m_copy(path2, dest2);
        }
        if (res != FR_OK) break;
    }
    f_closedir(&dir);
 //   gpio_put(PICO_DEFAULT_LED_PIN, false);
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
    draw_box((MAX_WIDTH - 60) / 2, 7, 60, 10, "Are you sure?", &lines);
    bool yes = true;
    draw_button((MAX_WIDTH - 60) / 2 + 16, 12, 11, "Yes", yes);
    draw_button((MAX_WIDTH - 60) / 2 + 35, 12, 10, "No", !yes);
    while(1) {
        if (is_dendy_joystick || is_kbd_joystick) {
            if (is_dendy_joystick) nespad_read();
            if (nespad_state_delay > 0) {
                nespad_state_delay--;
                sleep_ms(1);
            }
            else {
                nespad_state_delay = DPAD_STATE_DELAY;
                if(nespad_state & DPAD_UP) {
                    upPressed = true;
                } else if(nespad_state & DPAD_DOWN) {
                    downPressed = true;
                } else if (nespad_state & DPAD_A) {
                    enterPressed = true;
                } else if (nespad_state & DPAD_B) {
                    escPressed = true;
                } else if (nespad_state & DPAD_LEFT) {
                    leftPressed = true;
                } else if (nespad_state & DPAD_RIGHT) {
                    rightPressed = true;
                } else if (nespad_state & DPAD_SELECT) {
                    tabPressed = true;
                }
            }
        }
        if (enterPressed) {
            enterPressed = false;
            scan_code_cleanup();
            return yes;
        }
        if (tabPressed || leftPressed || rightPressed) { // TODO: own msgs cycle
            yes = !yes;
            draw_button((MAX_WIDTH - 60) / 2 + 16, 12, 11, "Yes", yes);
            draw_button((MAX_WIDTH - 60) / 2 + 35, 12, 10, "No", !yes);
            tabPressed = leftPressed = rightPressed = false;
            scan_code_cleanup();
        }
        if (escPressed) {
            escPressed = false;
            scan_code_cleanup();
            return false;
        }
    }
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
//    gpio_put(PICO_DEFAULT_LED_PIN, true);
    file_info_t* fp = selected_file();
    if (!fp) {
       no_selected_file();
       return;
    }
    char path[256];
    file_panel_desc_t* dsp = psp == left_panel ? right_panel : left_panel;
    snprintf(path, 256, "Copy %s %s to %s?", fp->s_name->p, fp->fattrib & AM_DIR ? "folder" : "file", dsp->s_path->p);
    if (m_prompt(path)) { // TODO: ask name
        construct_full_name(path, psp->s_path->p, fp->s_name->p);
        char dest[256];
        construct_full_name(dest, dsp->s_path->p, fp->s_name->p);
        FRESULT result = fp->fattrib & AM_DIR ? m_copy_recursive(path, dest) : m_copy(path, dest);
        if (result != FR_OK) {
            snprintf(line, MAX_WIDTH, "FRESULT: %d", result);
            const line_t lns[3] = {
                { -1, "Unable to copy selected item!" },
                { -1, path },
                { -1, line }
            };
            const lines_t lines = { 3, 2, lns };
            draw_box((MAX_WIDTH - 60) / 2, 7, 60, 10, "Error", &lines);
            sleep_ms(2500);
        }
    }
    redraw_window();
  //  gpio_put(PICO_DEFAULT_LED_PIN, false);
}

static void turn_usb_off(uint8_t cmd);
static void turn_usb_on(uint8_t cmd);

static void mark_to_exit(uint8_t cmd) {
    f10Pressed = false;
    escPressed = false;
    mark_to_exit_flag = true;
}

static void m_info(uint8_t cmd) {
    line_t plns[2] = {
        { 1, " It is ZX Murmulator OS Commander" },
        { 1, "tba" }
    };
    lines_t lines = { 2, 0, plns };
    draw_box(5, 2, MAX_WIDTH - 15, MAX_HEIGHT - 6, "Help", &lines);
    enterPressed = escPressed = false;
    nespad_state_delay = DPAD_STATE_DELAY;
    while(!escPressed && !enterPressed) {
        if (is_dendy_joystick || is_kbd_joystick) {
            if (is_dendy_joystick) nespad_read();
            if (nespad_state && !(nespad_state & DPAD_START) && !(nespad_state & DPAD_SELECT)) {
                nespad_state_delay = DPAD_STATE_DELAY;
                break;
            }
        }
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
    char dir[256];
    construct_full_name(dir, psp->s_path->p, "_");
    size_t len = strnlen(dir, 256) - 1;
    draw_panel(20, MAX_HEIGHT / 2 - 3, MAX_WIDTH - 40, 5, "DIR NAME", 0);
    draw_label(22, MAX_HEIGHT / 2 - 1, MAX_WIDTH - 44, dir, true, true);
    while(1) {
        char c = getch();
        if (escPressed || c == 0x1B) {
            escPressed = false;
            scan_code_cleanup();
            redraw_window();
            return;
        }
        if (backspacePressed || c == 8) {
            backspacePressed = false;
            scan_code_cleanup();
            if (len == 0) continue;
            dir[len--] = 0;
            dir[len] = '_';
            draw_label(22, MAX_HEIGHT / 2 - 1, MAX_WIDTH - 44, dir, true, true);
        }
        if (enterPressed || c == '\n') {
            enterPressed = false;
            break;
        }
        uint8_t sc = (uint8_t)lastCleanableScanCode;
        scan_code_cleanup();
        if (!sc || sc >= 0x80) continue;
       // char c = scan_code_2_cp866[sc]; // TODO: shift, caps lock, alt, rus...
        if (!c) continue;
        if (len + 2 == MAX_WIDTH - 44) continue;
        dir[len++] = c;
        dir[len] = '_';
        dir[len + 1] = 0;
        draw_label(22, MAX_HEIGHT / 2 - 1, MAX_WIDTH - 44, dir, true, true);
    }
    if (len) {
        dir[len] = 0;
        f_mkdir(dir);
    }
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
  //  gpio_put(PICO_DEFAULT_LED_PIN, true);
    file_info_t* fp = selected_file();
    if (!fp) {
       no_selected_file();
       return;
    }
    char path[256];
    file_panel_desc_t* dsp = psp == left_panel ? right_panel : left_panel;
    snprintf(path, 256, "Move %s %s to %s?", fp->s_name->p, fp->fattrib & AM_DIR ? "folder" : "file", dsp->s_path->p);
    if (m_prompt(path)) { // TODO: ask name
        construct_full_name(path, psp->s_path->p, fp->s_name->p);
        char dest[256];
        construct_full_name(dest, dsp->s_path->p, fp->s_name->p);
        FRESULT result = f_rename(path, dest);
        if (result != FR_OK) {
            snprintf(line, MAX_WIDTH, "FRESULT: %d", result);
            const line_t lns[3] = {
                { -1, "Unable to move selected item!" },
                { -1, path },
                { -1, line }
            };
            const lines_t lines = { 3, 2, lns };
            draw_box((MAX_WIDTH - 60) / 2, 7, 60, 10, "Error", &lines);
            sleep_ms(2500);
        }
    }
    redraw_window();
//    gpio_put(PICO_DEFAULT_LED_PIN, false);
}

void m_view(uint8_t nu) {
    if (hidePannels) return;
    file_info_t* fp = selected_file();
    if (!fp) return; // warn?
    if (fp->fattrib & AM_DIR) {
        enter_pressed();
        return;
    }
    static const char cstr[] = "mcview \"";
    strncpy(cmd, cstr, 512);
    construct_full_name(cmd + sizeof(cstr) - 1, psp->s_path->p, fp->s_name->p);
    size_t sz = strlen(cmd);
    cmd[sz++] = '\"';
    cmd[sz] = 0;
    draw_cmd_line(0, CMD_Y_POS);
    mark_to_exit_flag = cmd_enter(get_cmd_ctx(), cmd);
}

void m_edit(uint8_t nu) {
    if (hidePannels) return;
    file_info_t* fp = selected_file();
    if (!fp) return; // warn?
    if (fp->fattrib & AM_DIR) {
        enter_pressed();
        return;
    }
    static const char cstr[] = "mcedit \"";
    strncpy(cmd, cstr, 512);
    construct_full_name(cmd + sizeof(cstr) - 1, psp->s_path->p, fp->s_name->p);
    size_t sz = strlen(cmd);
    cmd[sz++] = '\"';
    cmd[sz] = 0;
    draw_cmd_line(0, CMD_Y_POS);
    mark_to_exit_flag = cmd_enter(get_cmd_ctx(), cmd);
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
    ' ', '3', "      ", do_nothing,
    ' ', '4', " Edit ", m_edit,
    ' ', '5', " Copy ", m_copy_file,
    ' ', '6', " Move ", m_move_file,
    ' ', '7', "      ", do_nothing,
    ' ', '8', " Del  ", m_delete_file,
    ' ', '9', "      ", do_nothing,
    '1', '0', " Exit ", mark_to_exit,
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
    draw_cmd_line(0, CMD_Y_POS);
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
    draw_cmd_line(0, CMD_Y_POS);
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
    return strcmp(e1->s_name->p, e2->s_name->p);
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
    if (!fp->s_name) {
        fp->s_name = new_string_cc(fi->fname);
    } else {
        string_replace_cs(fp->s_name, fi->fname);
    }
}

static void draw_cmd_line(int left, int top) {
    for (size_t i = left; i < MAX_WIDTH; ++i) {
        draw_text(" ", i, top, pcs->FOREGROUND_CMD_COLOR, pcs->BACKGROUND_CMD_COLOR);
    }
    graphics_set_con_pos(left, top);
    graphics_set_con_color(13, 0);
    printf("[%s]", get_ctx_var(get_cmd_ctx(), "CD"));
    graphics_set_con_color(7, 0);
    printf("> %s", cmd);
    graphics_set_con_color(pcs->FOREGROUND_CMD_COLOR, pcs->BACKGROUND_CMD_COLOR);
}

static void collect_files(file_panel_desc_t* p) {
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
    if (!m_opendir(&dir, p->s_path->p)) return;
    FILINFO fileInfo;
    while(f_readdir(&dir, &fileInfo) == FR_OK && fileInfo.fname[0] != '\0') {
        m_add_file(&fileInfo);
    }
    f_closedir(&dir);
    qsort(files_info, files_count, sizeof(file_info_t), m_comp);
}

static void construct_full_name(char* dst, const char* folder, const char* file) {
    if (strlen(folder) > 1) {
        snprintf(dst, 256, "%s/%s", folder, file);
    } else {
        snprintf(dst, 256, "/%s", file);
    }
}

static void fill_panel(file_panel_desc_t* p) {
    if (hidePannels) return;
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
    if (start_file_offset == 0 && p->s_path->size > 1) {
        draw_label(p->left + 1, y, p->width - 2, "..", p == psp && selected_file_idx == y, true);
        y++;
        p->files_number++;
    }
    for(int fn = 0; fn < files_count; ++ fn) {
        file_info_t* fp = &files_info[fn];
        if (start_file_offset <= p->files_number && y <= LAST_FILE_LINE_ON_PANEL_Y) {
            char* filename = fp->s_name->p;
            snprintf(line, MAX_WIDTH >> 1, "%s/%s", p->s_path->p, filename);
            bool selected = p == psp && selected_file_idx == y;
            draw_label(p->left + 1, y, p->width - 2, filename, selected, fp->fattrib & AM_DIR);
            y++;
        }
        p->files_number++;
    }
    for (; y <= LAST_FILE_LINE_ON_PANEL_Y; ++y) {
        draw_label(p->left + 1, y, p->width - 2, "", false, false);
    }
    if (p == psp) {
        set_ctx_var(get_cmd_ctx(), "CD", psp->s_path->p);
        draw_cmd_line(0, CMD_Y_POS);
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

inline static void handleJoystickEmulation(uint8_t sc) { // core 1
    if (!is_kbd_joystick) return;
    // DBGM_PRINT(("handleJoystickEmulation: %02Xh", sc));
    switch(sc) {
        case 0x1E: // A DPAD_A 
            nespad_state |= DPAD_A;
            break;
        case 0x9E:
            nespad_state &= ~DPAD_A;
            break;
        case 0x11: // W
            nespad_state2 |= DPAD_A;
            break;
        case 0x91:
            nespad_state2 &= ~DPAD_A;
            break;
        case 0x20: // D START
            nespad_state |= DPAD_START;
            break;
        case 0xA0:
            nespad_state &= ~DPAD_START;
            break;
        case 0x1F: // S SELECT 
            nespad_state |= DPAD_SELECT;
            break;
        case 0x9F:
            nespad_state &= ~DPAD_SELECT;
            break;
        case 0x2C: // Z
            nespad_state2 |= DPAD_B;
            break;
        case 0xAC:
            nespad_state2 &= ~DPAD_B;
            break;
        case 0x2D: // X
            nespad_state2 |= DPAD_SELECT;
            break;
        case 0xAD:
            nespad_state2 &= ~DPAD_SELECT;
            break;
        case 0x18: // O
            nespad_state2 |= DPAD_START;
            break;
        case 0x98:
            nespad_state2 &= ~DPAD_START;
            break;
        case 0x25: // K
            nespad_state2 |= DPAD_UP;
            break;
        case 0xA5:
            nespad_state2 &= ~DPAD_UP;
            break;
        case 0x27: // ;
            nespad_state |= DPAD_DOWN;
            break;
        case 0xA7:
            nespad_state &= ~DPAD_DOWN;
            break;
        case 0x26: // L
            nespad_state |= DPAD_LEFT;
            break;
        case 0xA6:
            nespad_state &= ~DPAD_LEFT;
            break;
        case 0x28: // ,(")
            nespad_state |= DPAD_RIGHT;
            break;
        case 0xA8:
            nespad_state &= ~DPAD_RIGHT;
            break;
        case 0x34: // .
            nespad_state2 |= DPAD_DOWN;
            break;
        case 0xB4:
            nespad_state2 &= ~DPAD_DOWN;
            break;
        case 0x10: // Q DPAD_B
            nespad_state |= DPAD_B;
            break;
        case 0x90:
            nespad_state &= ~DPAD_B;
            break;
        case 0x12: // E
            nespad_state2 |= DPAD_LEFT;
            break;
        case 0x92:
            nespad_state2 &= ~DPAD_LEFT;
            break;
        case 0x17: // I
            nespad_state2 |= DPAD_RIGHT;
            break;
        case 0x97:
            nespad_state2 &= ~DPAD_RIGHT;
            break;
        case 0x19: // P
            nespad_state |= DPAD_UP;
            break;
        case 0x99:
            nespad_state &= ~DPAD_UP;
            break;
        default:
            return;
    }
}

static scancode_handler_t scancode_handler;

static bool scancode_handler_impl(const uint32_t ps2scancode) { // core ?
    handleJoystickEmulation((uint8_t)ps2scancode);
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
    } else if (ps2scancode == 0xE01C) {
        enterPressed = true;
        goto r;
    } else if (ps2scancode == 0xE09C) {
        enterPressed = false;
        goto r;
    }
    switch ((uint8_t)ps2scancode & 0xFF) {
      case 0x01: // Esc down
        escPressed = true;
        break;
      case 0x81: // Esc up
        escPressed = false; break;
      case 0x4B: // left
        leftPressed = true; break;
      case 0xCB: // left
        leftPressed = false; break;
      case 0x4D: // right
        rightPressed = true;  break;
      case 0xCD: // right
        rightPressed = false;  break;
      case 0x38:
        altPressed = true;
        break;
      case 0xB8:
        altPressed = false;
        break;
      case 0x0E:
        backspacePressed = true;
        break;
      case 0x8E:
        backspacePressed = false;
        break;
      case 0x1C:
        enterPressed = true;
        break;
      case 0x9C:
        enterPressed = false;
        break;
      case 0x0C: // -
      case 0x4A: // numpad -
        minusPressed = true;
        break;
      case 0x8C: // -
      case 0xCA: // numpad 
        minusPressed = false;
        break;
      case 0x0D: // +=
      case 0x4E: // numpad +
        plusPressed = true;
        break;
      case 0x8D: // += 82?
      case 0xCE: // numpad +
        plusPressed = false;
        break;
      case 0x1D:
        ctrlPressed = true;
        break;
      case 0x9D:
        ctrlPressed = false;
        break;
      case 0x20:
        dPressed = true;
        break;
      case 0xA0:
        dPressed = false;
        break;
      case 0x2E:
        cPressed = true;
        break;
      case 0xAE:
        cPressed = false;
        break;
      case 0x14:
        tPressed = true;
        break;
      case 0x94:
        tPressed = false;
        break;
      case 0x22:
        gPressed = true;
        break;
      case 0xA2:
        gPressed = false;
        break;
      case 0x1E:
        aPressed = true;
        break;
      case 0x9E:
        aPressed = false;
        break;
      case 0x1F:
        sPressed = true;
        break;
      case 0x9F:
        sPressed = false;
        break;
      case 0x2D:
        xPressed = true;
        break;
      case 0xAD:
        xPressed = false;
        break;
      case 0x12:
        ePressed = true;
        break;
      case 0x92:
        ePressed = false;
        break;
      case 0x16:
        uPressed = true;
        break;
      case 0x96:
        uPressed = false;
        break;
      case 0x23:
        hPressed = true;
        break;
      case 0xA3:
        hPressed = false;
        break;
      case 0x3B: // F1..10 down
        f1Pressed = true; break;
      case 0x3C: // F2
        f2Pressed = true; break;
      case 0x3D: // F3
        f3Pressed = true; break;
      case 0x3E: // F4
        f4Pressed = true; break;
      case 0x3F: // F5
        f5Pressed = true; break;
      case 0x40: // F6
        f6Pressed = true; break;
      case 0x41: // F7
        f7Pressed = true; break;
      case 0x42: // F8
        f8Pressed = true; break;
      case 0x43: // F9
        f9Pressed = true; break;
      case 0x44: // F10
        f10Pressed = true; break;
      case 0x57: // F11
        f11Pressed = true; break;
      case 0x58: // F12
        f12Pressed = true; break;
      case 0xBB: // F1..10 up
        f1Pressed = false; break;
      case 0xBC: // F2
        f2Pressed = false; break;
      case 0xBD: // F3
        f3Pressed = false; break;
      case 0xBE: // F4
        f4Pressed = false; break;
      case 0xBF: // F5
        f5Pressed = false; break;
      case 0xC0: // F6
        f6Pressed = false; break;
      case 0xC1: // F7
        f7Pressed = false; break;
      case 0xC2: // F8
        f8Pressed = false; break;
      case 0xC3: // F9
        f9Pressed = false; break;
      case 0xC4: // F10
        f10Pressed = false; break;
      case 0xD7: // F11
        f11Pressed = false; break;
      case 0xD8: // F12
        f12Pressed = false; break;
      case 0x53: // Del down
        delPressed = true; break;
      case 0xD3: // Del up
        delPressed = false; break;
      case 0x39:
        spacePressed = true; break;
      case 0xB9:
        spacePressed = false; break;
      case 0x0F:
        tabPressed = true;
        break;
      case 0x8F:
//        left_panel_make_active = !left_panel_make_active; // TODO: combinations?
        tabPressed = false;
        break;
      default:
        //DBGM_PRINT(("handleScancode default: %02Xh", ps2scancode));
        break;
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
        set_ctx_var(get_cmd_ctx(), "CD", psp->s_path->p);
    }
    draw_cmd_line(0, CMD_Y_POS);
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

inline static int tokenize_cmd(char* cmdt, cmd_ctx_t* ctx) {
    while (!cmdt[0 && cmdt[0] == ' ']) ++cmdt; // ignore trailing spaces
    if (cmdt[0] == 0) {
        return 0;
    }
    if (ctx->orig_cmd) free(ctx->orig_cmd);
    ctx->orig_cmd = copy_str(cmdt);
    //goutf("orig_cmd: '%s' [%p]; cmd: '%s' [%p]\n", ctx->orig_cmd, ctx->orig_cmd, cmdt, cmdt);
    bool in_space = true;
    bool in_qutas = false;
    int inTokenN = 0;
    char* t1 = ctx->orig_cmd;
    char* t2 = cmdt;
    while(*t1) {
        if (*t1 == '"') in_qutas = !in_qutas;
        if (in_qutas) {
            *t2++ = *t1++;
            continue; 
        }
        char c = replace_spaces0(*t1++);
        //goutf("%02X -> %c %02X; t1: '%s' [%p], t2: '%s' [%p]\n", c, *t2, *t2, t1, t1, t2, t2);
        if (in_space) {
            if(c) { // token started
                in_space = 0;
                inTokenN++; // new token
            }
        } else if(!c) { // not in space, after the token
            in_space = true;
        }
        *t2++ = c;
    }
    *t2 = 0;
    //goutf("cmd: %s\n", cmd);
    return inTokenN;
}

inline static bool prepare_ctx(char* cmdt, cmd_ctx_t* ctx) {
    char* t = cmdt;
    bool in_quotas = false;
    bool append = false;
    char* std_out = 0;
    while (*t) {
        if (*t == '"') in_quotas = !in_quotas;
        if (!in_quotas && *t == '>') {
            *t++ = 0;
            if (*t == '>') {
                *t++ = 0;
                append = true;
                std_out = t;
            } else {
                std_out = t;
            }
            break;
        }
        t++;
    }
    if (std_out) {
        char* b = std_out;
        in_quotas = false;
        bool any_legal = false;
        while(*b) {
            if (!in_quotas && *b == ' ') {
                if (any_legal) {
                    *b = 0;
                    break;
                }
                std_out = b + 1;
            } else if (*b == '"') {
                if (in_quotas) {
                    *b = 0;
                    break;
                } else {
                    std_out = b + 1;
                }
                in_quotas = !in_quotas;
            } else {
                any_legal = true;
            }
            b++;
        }
        ctx->std_out = calloc(1, sizeof(FIL));
        if (FR_OK != f_open(ctx->std_out, std_out, FA_WRITE | (append ? FA_OPEN_APPEND : FA_CREATE_ALWAYS))) {
            printf("Unable to open file: '%s'\n", std_out);
            return false;
        }
    }

    int tokens = tokenize_cmd(cmdt, ctx);
    if (tokens == 0) {
        return false;
    }

    ctx->argc = tokens;
    ctx->argv = (char**)malloc(sizeof(char*) * tokens);
    t = cmdt;
    while (!t[0 && t[0] == ' ']) ++t; // ignore trailing spaces
    for (uint32_t i = 0; i < tokens; ++i) {
        ctx->argv[i] = copy_str(t);
        t = next_token(t);
        char *q = t;
        bool in_quotas = false;
        while (*q) {
            if (*q == '"') {
                if(in_quotas) {
                    *q = 0;
                    break;
                }
                else t = q + 1;
                in_quotas = !in_quotas;
            }
            q++;
        }
        
    }
    ctx->stage = PREPARED;
    return true;
}

inline static void cmd_write_history(cmd_ctx_t* ctx) {
    char* tmp = get_ctx_var(ctx, "TEMP");
    if(!tmp) tmp = "";
    size_t cdl = strlen(tmp);
    char * cmd_history_file = concat(tmp, ".cmd_history");
    FIL* pfh = (FIL*)malloc(sizeof(FIL));
    f_open(pfh, cmd_history_file, FA_OPEN_ALWAYS | FA_WRITE | FA_OPEN_APPEND);
    UINT br;
    f_write(pfh, cmd, strlen(cmd), &br);
    f_write(pfh, "\n", 1, &br);
    f_close(pfh);
    free(pfh);
    free(cmd_history_file);
}

static bool cmd_enter(cmd_ctx_t* ctx, const char* cmd) {
    putc('\n');
    size_t cmd_pos = strlen(cmd);
    if (cmd_pos) {
        cmd_write_history(ctx);
    } else {
        goto r2;
    }
    char* ccmd = copy_str(cmd);
    char* tc = cmd;
    char* ts = cmd;
    bool exit = false;
    bool in_qutas = false;
    cmd_ctx_t* ctxi = ctx;
    while(1) {
        if (!*tc) {
            //printf("'%s' by end zero\n", ts);
            exit = prepare_ctx(ts, ctxi);
            break;
        } else if (*tc == '"') {
            in_qutas = !in_qutas;
        } else if (!in_qutas && *tc == '|') {
            //printf("'%s' by pipe\n", ts);
            *tc = 0;
            cmd_ctx_t* curr = ctxi;
            cmd_ctx_t* next = new_ctx(ctxi);
            exit = prepare_ctx(ts, curr);
            curr->std_out = calloc(1, sizeof(FIL));
            curr->std_err = curr->std_out;
            next->std_in = calloc(1, sizeof(FIL));
            f_open_pipe(curr->std_out, next->std_in);
            curr->detached = true;
            next->prev = curr;
            curr->next = next;
            next->detached = false;
            ctxi = next;
            ts = tc + 1;
        } else if (!in_qutas && *tc == '&') {
            //printf("'%s' detached\n", ts);
            *tc = 0;
            exit = prepare_ctx(ts, ctxi);
            ctxi->detached = true;
            break;
        }
        tc++;
    }
    strncpy(cmd, ccmd, 512);
    free(ccmd);
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
    draw_cmd_line(0, CMD_Y_POS);
    return false;
}

static void enter_pressed() {
    size_t cmd_pos = strlen(cmd);
    if (cmd_pos && !ctrlPressed) {
        mark_to_exit_flag = cmd_enter(get_cmd_ctx(), cmd); // TODO: support "no exit" mode
        return;
    }
    if (hidePannels) return;
    file_info_t* fp = selected_file();
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
    char path[256];
    if (fp->fattrib & AM_DIR) {
        construct_full_name(path, psp->s_path->p, fp->s_name->p);
        string_replace_cs(psp->s_path, path);
        psp->level++;
        if (psp->level >= 16) {
            psp->level = 15;
        }
        psp->indexes[psp->level].selected_file_idx = FIRST_FILE_LINE_ON_PANEL_Y;
        psp->indexes[psp->level].start_file_offset = 0;
        psp->indexes[psp->level].dir_num = fp->dir_num;
        redraw_current_panel();
        return;
    }
    if (ctrlPressed) {
        construct_full_name(cmd + cmd_pos, psp->s_path->p, fp->s_name->p);
        draw_cmd_line(0, CMD_Y_POS);
        return;
    }
    construct_full_name(path, psp->s_path->p, fp->s_name->p);
    printf(path);
    strncpy(cmd, path, 256);
    mark_to_exit_flag = cmd_enter(get_cmd_ctx(), cmd); // TODO: support "no exit" mode
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
    char* tmp = get_ctx_var(ctx, "TEMP");
    if(!tmp) tmp = "";
    size_t cdl = strlen(tmp);
    char * cmd_history_file = concat(tmp, ".cmd_history");
    FIL* pfh = (FIL*)malloc(sizeof(FIL));
    size_t j = 0;
    int idx = 0;
    UINT br;
    f_open(pfh, cmd_history_file, FA_READ);
    char* b = malloc(512);
    while(f_read(pfh, b, 512, &br) == FR_OK && br) {
        for(size_t i = 0; i < br; ++i) {
            char t = b[i];
            if(t == '\n') { // next line
                cmd[j] = 0;
                j = 0;
                if(cmd_history_idx == idx)
                    break;
                idx++;
            } else {
                cmd[j++] = t;
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
    size_t cmd_pos = strlen(cmd);
    while(cmd_pos) {
        cmd[--cmd_pos] = 0;
        gbackspace();
    }
}

inline static void cmd_up(cmd_ctx_t* ctx) {
    cancel_entered();
    cmd_history_idx--;
    int idx = history_steps(ctx);
    if (cmd_history_idx < 0) cmd_history_idx = idx;
    goutf(cmd);
}

inline static void cmd_down(cmd_ctx_t* ctx) {
    cancel_entered();
    if (cmd_history_idx == -2) cmd_history_idx = -1;
    cmd_history_idx++;
    history_steps(ctx);
    goutf(cmd);
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
    size_t cmd_pos = strlen(cmd);
    if (cmd_pos == 0) {
        // TODO: blimp
        return;
    }
    cmd[--cmd_pos] = 0;
    gbackspace();
}

inline static void cmd_push(char c) {
    size_t cmd_pos = strlen(cmd);
    if (cmd_pos >= 512) {
        // TODO: blimp
    }
    cmd[cmd_pos++] = c;
    cmd[cmd_pos] = 0;
    putc(c);
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
    size_t cmd_pos = strlen(cmd);
    if (cmd_pos >= 512) {
        // TODO: blimp
        return;
    }
    putc(c);
    cmd[cmd_pos++] = c;
    cmd[cmd_pos] = 0;
}

inline static void cmd_tab(cmd_ctx_t* ctx) {
    char * p = cmd;
    char * p2 = cmd;
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
    char* b = malloc(512);
    if (p != p2) {
        strncpy(b, p, p2 - p);
        b[p2 - p] = 0;
    }
    DIR* pdir = (DIR*)malloc(sizeof(DIR));
    FILINFO* pfileInfo = malloc(sizeof(FILINFO));
    //goutf("\nDIR: %s\n", p != p2 ? b : curr_dir);
    if (FR_OK != f_opendir(pdir, p != p2 ? b : get_ctx_var(ctx, "CD"))) {
        free(b);
        return;
    }
    int total_files = 0;
    while (f_readdir(pdir, pfileInfo) == FR_OK && pfileInfo->fname[0] != '\0') {
        p3 = next_on(p2, pfileInfo->fname, in_quotas);
        if (p3 != pfileInfo->fname) {
            strcpy(b, p3);
            total_files++;
            break; // TODO: variants
        }
        //goutf("p3: %s; p2: %s; fn: %s; cmd_t: %s; fls: %d\n", p3, p2, fileInfo.fname, b, total_files);
    }
    if (total_files == 1) {
        p3 = b;
        while (*p3) {
            type_char(*p3++);
        }
        if (in_quotas) {
            type_char('"');
        }
    } else {
        // TODO: blimp
    }
    free(b);
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
    char* tmp = get_ctx_var(ctx, "TEMP");
    if(!tmp) tmp = "";
    size_t cdl = strlen(tmp);
    char * mc_con_file = concat(tmp, ".mc.con");
    FIL* pfh = (FIL*)malloc(sizeof(FIL));
    if (FR_OK != f_open(pfh, mc_con_file, FA_READ)) {
        goto r;
    }
    char* b = get_buffer();
    UINT rb;
    for (size_t y = 0; y < MAX_HEIGHT - 2; ++y)  {
        f_read(pfh, b + MAX_WIDTH * y * 2, MAX_WIDTH * 2, &rb);
    }
    f_close(pfh);
r:
    free(pfh);
    free(mc_con_file);
}

inline static void save_console(cmd_ctx_t* ctx) {
    char* tmp = get_ctx_var(ctx, "TEMP");
    if(!tmp) tmp = "";
    size_t cdl = strlen(tmp);
    char * mc_con_file = concat(tmp, ".mc.con");
    FIL* pfh = (FIL*)malloc(sizeof(FIL));
    if (FR_OK != f_open(pfh, mc_con_file, FA_CREATE_ALWAYS | FA_WRITE)) {
        goto r;
    }
    char* b = get_buffer();
    UINT wb;
    for (size_t y = 0; y < MAX_HEIGHT - 2; ++y)  {
        f_write(pfh, b + MAX_WIDTH * y * 2, MAX_WIDTH * 2, &wb);
    }
    f_close(pfh);
r:
    free(pfh);
    free(mc_con_file);
}

inline static void hide_pannels() {
    hidePannels = !hidePannels;
    if (hidePannels) {
        restore_console(get_cmd_ctx());
    } else {
        save_console(get_cmd_ctx());
        m_window();
        fill_panel(left_panel);
        fill_panel(right_panel);
    }
}

inline static void save_rc() {
    cmd_ctx_t* ctx = get_cmd_ctx();
    char* tmp = get_ctx_var(ctx, "TEMP");
    if(!tmp) tmp = "";
    size_t cdl = strlen(tmp);
    char * mc_rc_file = concat(tmp, ".mc.rc2");
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
    char* tmp = get_ctx_var(ctx, "TEMP");
    if(!tmp) tmp = "";
    size_t cdl = strlen(tmp);
    char * mc_rc_file = concat(tmp, ".mc.rc2");
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

static inline void work_cycle(cmd_ctx_t* ctx) {
    uint8_t repeat_cnt = 0;
    for(;;) {
        char c = getch_now();
        nespad_stat(&nespad_state, &nespad_state2);
        if (c) {
            if (c == 8) cmd_backspace();
            else if (c == 17) handle_up_pressed();
            else if (c == 18) handle_down_pressed();
            else if (c == '\t') handle_tab_pressed();
            else if (c == '\n') enter_pressed();
            else if (ctrlPressed && (c == 'o' || c == 'O' || c == 0x99 /*Щ*/ || c == 0xE9 /*щ*/)) hide_pannels();
            else cmd_push(c);
        }

        if (is_dendy_joystick || is_kbd_joystick) {
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
                    } else {
                        reset(0);
                        return;
                    }
                } else if (nespad_state & DPAD_A) {
                    ctrlPressed = nespad_state & DPAD_SELECT;
                    enter_pressed();
                } else if ((nespad_state & DPAD_LEFT) || (nespad_state & DPAD_RIGHT)) {
                    handle_tab_pressed();
                }
            }
        }
        if (ctrlPressed && altPressed && delPressed) {
            ctrlPressed = altPressed = delPressed = false;
            reset(0);
            return;
        }
    //    if_swap_drives();
    //    if_overclock();
    //    if_sound_control();
        if (lastSavedScanCode != lastCleanableScanCode && lastSavedScanCode > 0x80) {
            repeat_cnt = 0;
        } else {
            repeat_cnt++;
            if (repeat_cnt > 0xFE && lastSavedScanCode < 0x80) {
               lastCleanableScanCode = lastSavedScanCode + 0x80;
               repeat_cnt = 0;
            }
        }
        //if (lastCleanableScanCode) DBGM_PRINT(("lastCleanableScanCode: %02Xh", lastCleanableScanCode));
        switch(lastCleanableScanCode) {
          case 0x01: // Esc down
          //  mark_to_exit(9);
          case 0x81: // Esc up
          //  scan_code_processed();
            break;
          case 0x3B: // F1..10 down
          case 0x3C: // F2
          case 0x3D: // F3
          case 0x3E: // F4
          case 0x3F: // F5
          case 0x40: // F6
          case 0x41: // F7
          case 0x42: // F8
          case 0x43: // F9
          case 0x44: // F10
          case 0x57: // F11
          case 0x58: // F12
            scan_code_processed();
            break;
          case 0xBB: // F1..10 up
          case 0xBC: // F2
          case 0xBD: // F3
          case 0xBE: // F4
          case 0xBF: // F5
          case 0xC0: // F6
          case 0xC1: // F7
          case 0xC2: // F8
          case 0xC3: // F9
          case 0xC4: // F10
          case 0xD7: // F11
          case 0xD8: // F12
            if (lastSavedScanCode != lastCleanableScanCode - 0x80) {
                break;
            }
            fn_1_12_btn_pressed(lastCleanableScanCode - 0xBB);
            scan_code_processed();
            break;
          case 0x1D: // Ctrl down
          case 0x9D: // Ctrl up
          case 0x38: // ALT down
          case 0xB8: // ALT up
            bottom_line();
            scan_code_processed();
            break;
        //  case 0x50: // down arr down
        //    scan_code_processed();
        //    break;
        //  case 0xD0: // down arr up
        //    if (lastSavedScanCode != 0x50) {
        //        break;
        //    }
        //    handle_down_pressed();
        //    break;
          case 0x49: // pageup arr down
            scan_code_processed();
            break;
          case 0xC9: // pageup arr up
            if (lastSavedScanCode != 0x49) {
                break;
            }
            handle_pageup_pressed();
            break;
          case 0x51: // pagedown arr down
            scan_code_processed();
            break;
          case 0xD1: // pagedown arr up
            if (lastSavedScanCode != 0x51) {
                break;
            }
            handle_pagedown_pressed();
            break;
        //  case 0x48: // up arr down
        //    scan_code_processed();
        //    break;
        //  case 0xC8: // up arr up
        //    if (lastSavedScanCode != 0x48) {
        //        break;
        //    }
        //    handle_up_pressed();
        //    break;
          case 0xCB: // left
            break;
          case 0xCD: // right
            break;
          case 0x1C: // Enter down
            scan_code_processed();
            break;
          case 0x9C: // Enter up
        //    if (lastSavedScanCode != 0x1C) {
        //        break;
        //    }
        //    enter_pressed();
            scan_code_processed();
            break;
        }
        /*
        if (usb_started && tud_msc_ejected()) {
            turn_usb_off(0);
        }
        if (usb_started) {
            pico_usb_drive_heartbeat();
        } else
        */
        if(mark_to_exit_flag) {
            save_rc();
            restore_console(ctx);
            draw_cmd_line(0, CMD_Y_POS);
            putc('\n');
            return;
        }
        // static char tt[] = "cleanable scan-code: %02Xh / saved scan-code: %02Xh";
        // snprintf(line, MAX_WIDTH, tt, lastCleanableScanCode, lastSavedScanCode);
        // draw_cmd_line(0, CMD_Y_POS, line);
        // sleep_ms(500);
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
    if (ctx->argc == 0) {
        fprintf(ctx->std_err, "ATTTENTION! BROKEN EXECUTION CONTEXT [%p]!\n", ctx);
        return -1;
    }
    MAX_WIDTH = get_console_width();
    MAX_HEIGHT = get_console_height();
    F_BTN_Y_POS = TOTAL_SCREEN_LINES - 1;
    CMD_Y_POS = F_BTN_Y_POS - 1;
    PANEL_LAST_Y = CMD_Y_POS - 1;
    line = calloc(MAX_WIDTH + 2, 1);
    LAST_FILE_LINE_ON_PANEL_Y = PANEL_LAST_Y - 1;
    save_console(ctx);

    cmd = malloc(512);
    cmd[0] = 0;

    left_panel = calloc(1, sizeof(file_panel_desc_t));
    left_panel->s_path = new_string_cc("/");
    right_panel = calloc(1, sizeof(file_panel_desc_t));
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

    files_info = calloc(sizeof(file_info_t), MAX_FILES);

    scancode_handler = get_scancode_handler();
    set_scancode_handler(scancode_handler_impl);

    start_manager(ctx);

    set_scancode_handler(scancode_handler);
    free(line);
    delete_string(right_panel->s_path);
    free(right_panel);
    delete_string(left_panel->s_path);
    free(left_panel);
    free(pcs);
    for (int i = 0; i < MAX_FILES; ++i) {
        if (files_info[i].s_name) delete_string(files_info[i].s_name); 
    }
    free(files_info);
    free(cmd);

    return 0;
}

int __required_m_api_verion(void) {
    return M_API_VERSION;
}
