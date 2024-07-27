#include "m-os-api.h"

static void m_window();
static void redraw_window();
static void draw_cmd_line(int left, int top);
static void bottom_line();
static void construct_full_name(char* dst, const char* folder, const char* file);
static bool m_prompt(const char* txt);
static void no_selected_file();

#define PANEL_TOP_Y 0
#define FIRST_FILE_LINE_ON_PANEL_Y (PANEL_TOP_Y + 1)
static uint32_t MAX_HEIGHT, MAX_WIDTH;
static uint8_t PANEL_LAST_Y, F_BTN_Y_POS, CMD_Y_POS, LAST_FILE_LINE_ON_PANEL_Y;
#define TOTAL_SCREEN_LINES MAX_HEIGHT
static char* line;
static char selected_file_path[260]; // TODO: ctx->

static volatile uint32_t lastCleanableScanCode;
static volatile uint32_t lastSavedScanCode;

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
static volatile bool left_panel_make_active = true;

// TODO:
static bool is_dendy_joystick = true;
static bool is_kbd_joystick = false;
#define DPAD_STATE_DELAY 200
static int nespad_state_delay = DPAD_STATE_DELAY;
static uint8_t nespad_state, nespad_state2;
static bool mark_to_exit_flag = false;

inline static void nespad_read() {
    // TODO:
    nespad_state = nespad_state2 = 0;
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
    char path[256];
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
    char*   pname; //[MAX_WIDTH >> 1];
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
    selected_file_path[0] = 0;

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
    left_panel_make_active = true;

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
    if (selected_file_idx == FIRST_FILE_LINE_ON_PANEL_Y && start_file_offset == 0 && strlen(psp->path) > 1) {
        return 0;
    }
    collect_files(psp);
    int y = 1;
    int files_number = 0;
    if (start_file_offset == 0 && strlen(psp->path) > 1) {
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

// TODO:
#define save_snap do_nothing
#define swap_drives do_nothing
#define switch_mode do_nothing
#define switch_color do_nothing
#define conf_it do_nothing
#define restore_snap do_nothing

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
    snprintf(path, 256, "Remove %s %s?", fp->pname, fp->fattrib & AM_DIR ? "folder" : "file");
    if (m_prompt(path)) {
        construct_full_name(path, psp->path, fp->pname);
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
    snprintf(path, 256, "Copy %s %s to %s?", fp->pname, fp->fattrib & AM_DIR ? "folder" : "file", dsp->path);
    if (m_prompt(path)) { // TODO: ask name
        construct_full_name(path, psp->path, fp->pname);
        char dest[256];
        construct_full_name(dest, dsp->path, fp->pname);
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
    if (tud_msc_ejected()) {
        mark_to_exit_flag = true;
    }
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
    construct_full_name(dir, psp->path, "_");
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
    snprintf(path, 256, "Move %s %s to %s?", fp->pname, fp->fattrib & AM_DIR ? "folder" : "file", dsp->path);
    if (m_prompt(path)) { // TODO: ask name
        construct_full_name(path, psp->path, fp->pname);
        char dest[256];
        construct_full_name(dest, dsp->path, fp->pname);
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

static void usb_task(void *pv) {
    while (!tud_msc_ejected()) {
        pico_usb_drive_heartbeat();
    }
    int post_cicles = 100;
    while (--post_cicles) {
        pico_usb_drive_heartbeat();
    }
    vTaskDelete(NULL);
}

static void turn_usb_off(uint8_t cmd) {
    if (tud_msc_ejected()) return;
// TODO: eject
    set_tud_msc_ejected(true);
    redraw_window();
}

static void turn_usb_on(uint8_t cmd) {
    if (!tud_msc_ejected()) return;
    init_pico_usb_drive();
    bottom_line();
    xTaskCreate(usb_task, "mc usb task", configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES - 1, NULL);
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
        if (fn_1_12_tbl[9].action != mark_to_exit) {
            // Alt + F10 no more actions
            sprintf(fn_1_12_tbl_alt[9].name, " USB  ");
            fn_1_12_tbl_alt[9].action = turn_usb_on;
            // F10 / Ctrl + F10 - Exit
            sprintf(fn_1_12_tbl[9].name, " Exit ");
            fn_1_12_tbl[9].action = mark_to_exit;
            sprintf(fn_1_12_tbl_ctrl[9].name, " Exit ");
            fn_1_12_tbl_ctrl[9].action = mark_to_exit;
        }
    } else {
        if (fn_1_12_tbl_alt[9].action != turn_usb_off) {
            // do not Exit in usb mode
            memset(fn_1_12_tbl[9].name, ' ', BTN_WIDTH);
            fn_1_12_tbl[9].action = do_nothing;
            memset(fn_1_12_tbl_ctrl[9].name, ' ', BTN_WIDTH);
            fn_1_12_tbl_ctrl[9].action = do_nothing;
            // Alt + F10 - force unmount usb
            snprintf(fn_1_12_tbl_alt[9].name, BTN_WIDTH, " UnUSB ");
            fn_1_12_tbl_alt[9].action = turn_usb_off;
        }
    }
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
    size_t sz = MAX_WIDTH >> 1; // strlen(fi->fname);
    // if (sz > (MAX_WIDTH >> 1)) sz = MAX_WIDTH >> 1;
    fp->pname = calloc(sz, 1);
    strncpy(fp->pname, fi->fname, sz);
}

static void draw_cmd_line(int left, int top) {
    for (size_t i = left; i < MAX_WIDTH; ++i) {
        draw_text(" ", i, top, pcs->FOREGROUND_CMD_COLOR, pcs->BACKGROUND_CMD_COLOR);
    }
    graphics_set_con_pos(left, top);
    graphics_set_con_color(13, 0);
    printf("[%s]", get_ctx_var(get_cmd_ctx(), "CD"));
    graphics_set_con_color(7, 0);
    printf("> ");
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
    if (!m_opendir(&dir, p->path)) return;
    FILINFO fileInfo;
    while(f_readdir(&dir, &fileInfo) == FR_OK && fileInfo.fname[0] != '\0') {
        m_add_file(&fileInfo);
    }
    f_closedir(&dir);
    qsort(files_info, files_count, sizeof(file_info_t), m_comp);
}

static void construct_full_name(char* dst, const char* folder, const char* file) {
    if (strlen(folder) > 1) {
        snprintf(dst, 256, "%s\\%s", folder, file);
    } else {
        snprintf(dst, 256, "\\%s", file);
    }
}

inline static void detect_os_type(const char* path, char* os_type, size_t sz) {
    // TODO:
    strncpy(os_type, path, sz);
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
        set_ctx_var(get_cmd_ctx(), "CD", psp->path);
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
    snprintf(line, MAX_WIDTH, "SD:%s", left_panel->path);
    draw_panel( 0, PANEL_TOP_Y, MAX_WIDTH / 2, PANEL_LAST_Y + 1, line, 0);
    snprintf(line, MAX_WIDTH, "SD:%s", right_panel->path);
    draw_panel(MAX_WIDTH / 2, PANEL_TOP_Y, MAX_WIDTH / 2, PANEL_LAST_Y + 1, line, 0);
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
        left_panel_make_active = !left_panel_make_active; // TODO: combinations?
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
    snprintf(line, MAX_WIDTH >> 1, "SD:%s", psp->path);
    draw_panel(psp->left, PANEL_TOP_Y, psp->width, PANEL_LAST_Y + 1, line, 0);
    fill_panel(psp);
    set_ctx_var(get_cmd_ctx(), "CD", psp->path);
    draw_cmd_line(0, CMD_Y_POS);
}

static inline void enter_pressed() {
    file_info_t* fp = selected_file();
    if (!fp) { // up to parent dir
        int i = strlen(psp->path);
        while(--i > 0) {
            if (psp->path[i] == '\\') {
                psp->path[i] = 0;
                psp->level--;
                redraw_current_panel();
                return;
            }
        }
        psp->path[0] = '\\';
        psp->path[1] = 0;
#if EXT_DRIVES_MOUNT
        psp->in_dos = false;
#endif
        psp->level--;
        redraw_current_panel();
        return;
    }
    if (fp->fattrib & AM_DIR) {
        char path[256];
        construct_full_name(path, psp->path, fp->pname);
        strncpy(psp->path, path, 256);
        psp->level++;
        if (psp->level >= 16) {
            psp->level = 15;
        }
        psp->indexes[psp->level].selected_file_idx = FIRST_FILE_LINE_ON_PANEL_Y;
        psp->indexes[psp->level].start_file_offset = 0;
        psp->indexes[psp->level].dir_num = fp->dir_num;
        redraw_current_panel();
        return;
    } else {
        /*
        size_t slen = strlen(fp->name);
        if (slen > 4 && fp->name[slen - 4] == '.')
        if(
            (fp->name[slen - 1] == 'N' || fp->name[slen - 1] == 'n') &&
            (fp->name[slen - 2] == 'I' || fp->name[slen - 2] == 'i') &&
            (fp->name[slen - 3] == 'B' || fp->name[slen - 3] == 'b')
        ) {
            char path[256];
            construct_full_name(path, psp->path, fp->name);
            run_bin(path);
            return;
        } else if (
            (fp->name[slen - 1] == 'G' || fp->name[slen - 1] == 'g') &&
            (fp->name[slen - 2] == 'M' || fp->name[slen - 2] == 'm') &&
            (fp->name[slen - 3] == 'I' || fp->name[slen - 3] == 'i')
        ) {
            char path[256];
            construct_full_name(path, psp->path, fp->name);
            run_img(path);
            return;
        } else if (
            (fp->name[slen - 1] == 'D' || fp->name[slen - 1] == 'd') &&
            (fp->name[slen - 2] == 'K' || fp->name[slen - 2] == 'k') &&
            (fp->name[slen - 3] == 'B' || fp->name[slen - 3] == 'b')
        ) {
            char path[256];
            construct_full_name(path, psp->path, fp->name);
            run_img(path); // TODO: separate support for .BKD
            return;
        } else if (
            (fp->name[slen - 1] == 'E' || fp->name[slen - 1] == 'e') &&
            (fp->name[slen - 2] == 'K' || fp->name[slen - 2] == 'k') &&
            (fp->name[slen - 3] == 'B' || fp->name[slen - 3] == 'b')
        ) {
            char path[256];
            construct_full_name(path, psp->path, fp->name);
            restore_snap_by_filename(path);
            return;
        }
        */
    }
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

inline static void handle_down_pressed() {
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

static inline void work_cycle() {
    uint8_t repeat_cnt = 0;
    for(;;) {
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
                } else if (nespad_state & DPAD_START) {
                    enter_pressed();
                } else if ((nespad_state & DPAD_A) && (nespad_state & DPAD_START)) {
                    turn_usb_on(0);
                } else if ((nespad_state & DPAD_B) && (nespad_state & DPAD_SELECT)) {
                    mark_to_exit(0);
                } else if ((nespad_state & DPAD_LEFT) || (nespad_state & DPAD_RIGHT)) {
                    left_panel_make_active = !left_panel_make_active;
                } else if ((nespad_state & DPAD_A) && (nespad_state & DPAD_SELECT)) {
                    conf_it(0);
                } else if ((nespad_state & DPAD_B) && (nespad_state & DPAD_START)) {
                    reset(0);
                    return;
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
        if (psp == left_panel && !left_panel_make_active) {
            select_right_panel();
        }
        if (psp != left_panel && left_panel_make_active) {
            select_left_panel();
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
          case 0x50: // down arr down
            scan_code_processed();
            break;
          case 0xD0: // down arr up
            if (lastSavedScanCode != 0x50) {
                break;
            }
            handle_down_pressed();
            break;
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
          case 0x48: // up arr down
            scan_code_processed();
            break;
          case 0xC8: // up arr up
            if (lastSavedScanCode != 0x48) {
                break;
            }
            handle_up_pressed();
            break;
          case 0xCB: // left
            break;
          case 0xCD: // right
            break;
          case 0x1C: // Enter down
            scan_code_processed();
            break;
          case 0x9C: // Enter up
            if (lastSavedScanCode != 0x1C) {
                break;
            }
            enter_pressed();
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
            return;
        }
        // static char tt[] = "cleanable scan-code: %02Xh / saved scan-code: %02Xh";
        // snprintf(line, MAX_WIDTH, tt, lastCleanableScanCode, lastSavedScanCode);
        // draw_cmd_line(0, CMD_Y_POS, line);
        // sleep_ms(500);
    }
}

inline static void start_manager() {
    m_window();
    select_left_panel();
    update_menu_color();
//        m_info(0); // F1 TODO: ensure it is not too aggressive
    work_cycle();
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

    start_manager();

    set_scancode_handler(scancode_handler);
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
