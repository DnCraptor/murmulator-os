#include "m-os-api.h"

typedef struct line {
   int8_t off;
   char* txt;
} line_t;

typedef struct lines {
   uint8_t sz;
   uint8_t toff;
   line_t* plns;
} lines_t;

static void m_window();
static void redraw_window();
static void bottom_line();
static bool m_prompt(const char* txt);

#define PANEL_TOP_Y 0
#define FIRST_FILE_LINE_ON_PANEL_Y (PANEL_TOP_Y + 1)
static uint32_t MAX_HEIGHT, MAX_WIDTH;
static uint8_t PANEL_LAST_Y, F_BTN_Y_POS, LAST_FILE_LINE_ON_PANEL_Y;
#define TOTAL_SCREEN_LINES MAX_HEIGHT

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
static volatile bool homePressed = false;
static volatile bool endPressed = false;
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

static size_t line_s = 0;
static size_t col_s = 0;
static size_t line_n = 0;
static size_t col_n = 0;
static size_t f_sz;
static list_t* lst;

inline static void nespad_read() {
    nespad_state = 0;
    nespad_state2 = 0;
//    nespad_stat(&nespad_state, &nespad_state2);
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
static color_schema_t* pcs;

void* memset(void* p, int v, size_t sz) {
    typedef void* (*fn)(void *, int, size_t);
    return ((fn)_sys_table_ptrs[142])(p, v, sz);
}

void* memcpy(void *__restrict dst, const void *__restrict src, size_t sz) {
    typedef void* (*fn)(void *, const void*, size_t);
    return ((fn)_sys_table_ptrs[167])(dst, src, sz);
}

int _init(void) {
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
    homePressed = false;
    endPressed = false;
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
    line_s = 0;
    col_s = 0;
    line_n = 0;
    col_n = 0;
    scan_code_cleanup();
}

static bool f_read_str(FIL* f, char* buf, size_t lim) { // TODO: API
    UINT br;
    if (f_read(f, buf, lim, &br) != FR_OK || br == 0) {
        return false;
    }
    if (buf[0] == '\r') {
        for (size_t i = 1; i < br; ++i) {
            buf[i - 1] = buf[i];
            if(buf[i] == '\n') {
               if (i != 1 && buf[i - 2] == '\r') buf[i - 2] = 0;
                buf[i - 1] = 0;
                f_lseek(f, f_tell(f) + i - br);
                return true;
            }
        }
    }
    for (size_t i = 0; i < br; ++i) {
        if(buf[i] == '\n') {
            if (i != 0 && buf[i - 1] == '\r') buf[i - 1] = 0;
            buf[i] = 0;
            f_lseek(f, f_tell(f) + i + 1 - br);
            return true;
        }
    }
    buf[br - 1] = 0;
    f_lseek(f, f_tell(f) - 1);
    return true;
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
    char line[32];
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
    draw_box((MAX_WIDTH - 60) / 2, 7, 60, 10, "Are you sure?", &lines);
    bool yes = true;
    draw_button(x + shift + 6, 12, 11, "Yes", yes);
    draw_button(x + shift + 25, 12, 10, "No", !yes);
    while(1) {
        char c = getch_now();
        if (c) {
            if (c == CHAR_CODE_ENTER) {
                enterPressed = false;
                scan_code_cleanup();
                return yes;
            }
            if (c == CHAR_CODE_ESC) {
                escPressed = false;
                scan_code_cleanup();
                return false;
            }
        }

        if (is_dendy_joystick || is_kbd_joystick) {
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
}

static void mark_to_exit(uint8_t cmd) {
    f10Pressed = false;
    escPressed = false;
    mark_to_exit_flag = true;
}

static void m_info(uint8_t cmd) {
    line_t plns[2] = {
        { 1, " It is ZX Murmulator OS Commander Editor" },
        { 1, " Let edit this file and do not miss to save it using F2 button." }
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

static void m_save(uint8_t cmd) {
    cmd_ctx_t* ctx = get_cmd_ctx();
    FIL* f = malloc(sizeof(FIL));
    if (FR_OK != f_open(f, ctx->argv[1], FA_CREATE_ALWAYS | FA_WRITE)) {
        free(f);
        // TODO: err
        return;
    }
    UINT bw;
    node_t* i = lst->first;
    while (i) {
        f_write(f, c_str(i->data), c_strlen(i->data), &bw);
        f_write(f, "\n", 1, &bw);
        i = i->next;
    }
    f_close(f);
    free(f);
}

static fn_1_12_tbl_t fn_1_12_tbl = {
    ' ', '1', " Info ", m_info,
    ' ', '2', " Save ", m_save,
    ' ', '3', "      ", do_nothing,
    ' ', '4', "      ", do_nothing,
    ' ', '5', "      ", do_nothing,
    ' ', '6', "      ", do_nothing,
    ' ', '7', "      ", do_nothing,
    ' ', '8', "      ", do_nothing,
    ' ', '9', "      ", do_nothing,
    '1', '0', " Exit ", mark_to_exit,
    '1', '1', "      ", do_nothing,
    '1', '2', "      ", do_nothing
};

static fn_1_12_tbl_t fn_1_12_tbl_alt = {
    ' ', '1', "      ", do_nothing,
    ' ', '2', "      ", do_nothing,
    ' ', '3', "      ", do_nothing,
    ' ', '4', "      ", do_nothing,
    ' ', '5', "      ", do_nothing,
    ' ', '6', "      ", do_nothing,
    ' ', '7', "      ", do_nothing,
    ' ', '8', "      ", do_nothing,
    ' ', '9', "      ", do_nothing,
    '1', '0', " Exit ", mark_to_exit,
    '1', '1', "      ", do_nothing,
    '1', '2', "      ", do_nothing
};

static fn_1_12_tbl_t fn_1_12_tbl_ctrl = {
    ' ', '1', "      ", do_nothing,
    ' ', '2', "      ", do_nothing,
    ' ', '3', "      ", do_nothing,
    ' ', '4', "      ", do_nothing,
    ' ', '5', "      ", do_nothing,
    ' ', '6', "      ", do_nothing,
    ' ', '7', "      ", do_nothing,
    ' ', '8', "      ", do_nothing,
    ' ', '9', "      ", do_nothing,
    '1', '0', " Exit ", mark_to_exit,
    '1', '1', "      ", do_nothing,
    '1', '2', "      ", do_nothing
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
    int i = 0;
    for (; i < BTNS_COUNT && (i + 1) * BTN_WIDTH < MAX_WIDTH; ++i) {
        const fn_1_12_tbl_rec_t* rec = &(*actual_fn_1_12_tbl())[i];
        draw_fn_btn(rec, i * BTN_WIDTH, F_BTN_Y_POS);
    }
    i = i * BTN_WIDTH;
    for (; i < MAX_WIDTH; ++i) {
        draw_text(" ", i, F_BTN_Y_POS, pcs->FOREGROUND_F1_12_COLOR, pcs->BACKGROUND_F1_12_COLOR);
    }
}

static void redraw_window() {
    m_window();
    bottom_line();
}

static void m_window() {
    if (hidePannels) return;
    char buff[64];
    cmd_ctx_t* ctx = get_cmd_ctx();
    draw_panel(0, PANEL_TOP_Y, MAX_WIDTH, PANEL_LAST_Y + 1, ctx->argv[1], 0);

    size_t y = 1;
    size_t line = 0;
    node_t* i = lst->first;
    string_t* s = new_string_v();
    while(i) {
        if (line >= line_s) {
            if (c_strlen(i->data) >= col_s) {
                string_replace_cs(s, c_str(i->data) + col_s);
                if (s->size > MAX_WIDTH - 2) {
                    s->size = MAX_WIDTH - 2;
                    s->p[s->size] = '>';
                    string_reseve(s, s->size + 2);
                    s->p[s->size + 1] = 0;
                }
                draw_text(s->p, 1, y, pcs->FOREGROUND_FIELD_COLOR, pcs->BACKGROUND_FIELD_COLOR);
            }
            y++;
            if (y > MAX_HEIGHT - 3) break;
        }
        i = i->next;
        ++line;
    }
    delete_string(s);

    size_t free_sz = xPortGetFreeHeapSize();
    size_t cnt = list_count(lst);
    f_sz = list_data_bytes(lst);
    if (f_sz > 10000) {
        snprintf(buff, 64, " [%d:%d] %dK lns: %d (%dK free) ", line_n, col_n, f_sz >> 10, cnt, free_sz >> 10);
    } else {
        snprintf(buff, 64, " [%d:%d] %dB lns: %d (%dK free) ", line_n, col_n, f_sz, cnt, free_sz >> 10);
    }
    draw_text(
        buff,
        2,
        PANEL_LAST_Y,
        pcs->FOREGROUND_FIELD_COLOR,
        pcs->BACKGROUND_FIELD_COLOR
    );
    graphics_set_con_pos(col_n - col_s + 1, line_n - line_s + 1);
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
    } else if (ps2scancode == 0xE047 || (ps2scancode == 0x47 && !numlock)) {
        homePressed = true;
        goto r;
    } else if (ps2scancode == 0xE0C7 || (ps2scancode == 0xC7 && !numlock)) {
        homePressed = false;
        goto r;
    } else if (ps2scancode == 0xE04F || (ps2scancode == 0x4F && !numlock)) {
        endPressed = true;
        goto r;
    } else if (ps2scancode == 0xE0CF || (ps2scancode == 0xCF && !numlock)) {
        endPressed = false;
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

inline static void handle_pagedown_pressed() {
    if (hidePannels) return;
    line_s += MAX_HEIGHT - 4;
    line_n += MAX_HEIGHT - 4;
    m_window();
}

inline static void handle_down_pressed() {
    if (hidePannels) return;
    ++line_n;
    if (line_n > line_s + MAX_HEIGHT - 5) {
        ++line_s;
    }
    m_window();
}

inline static void handle_pageup_pressed() {
    if (hidePannels) return;
    if (line_s <= MAX_HEIGHT - 4) {
        line_s = 0;
        line_n = 0;
    } else {
        line_s -= MAX_HEIGHT - 4;
        line_n -= MAX_HEIGHT - 4;
    }
    m_window();
}

inline static void handle_up_pressed() {
    if (hidePannels) return;
    if (line_n == line_s) {
        if (line_n >= 1) {
            --line_s;
            --line_n;
            m_window();
        }
        return;
    }
    --line_n;
    m_window();
}

inline static void enter_pressed() {
    if (hidePannels) return;
    node_t* n = list_get_node_at(lst, line_n);
    if (!n) {
        list_inject_till(lst, line_n);
    } else {
        string_t* s = string_split_at(n->data, col_n);
        list_inset_data_after(lst, n, s);
    }
    col_n = 0;
    col_s = 0;
    handle_down_pressed();
}

inline static fn_1_12_btn_pressed(uint8_t fn_idx) {
    if (fn_idx > 11) fn_idx -= 18; // F11-12
    (*actual_fn_1_12_tbl())[fn_idx].action(fn_idx);
}

inline static void push_char(char c) {
    string_t* s = list_get_data_at(lst, line_n);
    if (!s) {
        s = list_inject_till(lst, line_n)->data;
    }
    string_insert_c(s, c, col_n);
    ++col_n;
    if (col_n > col_s + MAX_WIDTH - 3) {
        ++col_s;
    }
    m_window();
}

inline static void cmd_backspace() {
    if (col_n == 0) {
        if (line_n == 0) {
            return;
        }
        node_t* n = list_get_node_at(lst, line_n);
        if (!n || !n->prev) {
            --line_n;
            m_window();
            return;
        }
        string_t* s1 = n->prev->data;
        col_n = s1->size;
        while (col_n > col_s + MAX_WIDTH - 3) {
            ++col_s;
        }
        string_push_back_cs(s1, n->data);
        list_erase_node(lst, n);
        --line_n;
        if (line_n < line_s) line_s = line_n;
        m_window();
        return;
    }
    string_t* s = list_get_data_at(lst, line_n);
    if (!s) {
        s = list_inject_till(lst, line_n)->data;
    }
    if (s->size > 0 && col_n > 0) {
        if (col_n == col_s) {
            --col_s;
        }
        --col_n;
        string_clip(s, col_n);
    } else if (line_n > 0) {
        --line_n;
    }
    m_window();
}

inline static void cmd_del() {
    node_t* n = list_get_node_at(lst, line_n);
    if (!n) {
        return;
    }
    string_t* s = n->data;
    if (col_n >= s->size) {
        if (!n->next) {
            return;
        }
        while (col_n != s->size) { // todo: memset
            string_push_back_c(s, ' ');
        }
        string_push_back_cs(s, n->next->data);
        list_erase_node(lst, n->next);
        m_window();
        return;
    }
    if (s->size) {
        string_clip(s, col_n);
    }
    m_window();
}

inline static void handle_right_pressed(void) {
    if (hidePannels) {
        return;
    }
    ++col_n;
    if (col_n > col_s + MAX_WIDTH - 3) {
        ++col_s;
    }
    m_window();
}

inline static void handle_left_pressed(void) {
    if (hidePannels || col_n == 0) {
        return;
    }
    if (col_s == col_n) {
        --col_s;
    }
    --col_n;
    m_window();
}

inline static void handle_home_pressed(void) {
    col_s = 0;
    col_n = 0;
    m_window();
}

inline static void handle_end_pressed(void) {
    string_t* s = list_get_data_at(lst, line_n);
    if (!s) {
        col_s = 0;
        col_n = 0;
    } else {
        col_n = s->size;
        if (col_n > col_s + MAX_WIDTH - 3) {
            col_s = col_n - MAX_WIDTH + 3;
        }
    }
    m_window();
}

inline static void handle_tab_pressed(void) {
    if (hidePannels) {
        return;
    }
    string_t* s = list_get_data_at(lst, line_n);
    if (!s) {
        s = list_inject_till(lst, line_n)->data;
    }
    string_insert_c(s, ' ', col_n);
    ++col_n;
    string_insert_c(s, ' ', col_n);
    ++col_n;
    string_insert_c(s, ' ', col_n);
    ++col_n;
    string_insert_c(s, ' ', col_n);
    ++col_n;
    if (col_n > col_s + MAX_WIDTH - 3) {
        col_s = col_n - MAX_WIDTH + 3;
    }
    m_window();
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
    for (size_t y = 0; y < MAX_HEIGHT - 1; ++y)  {
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
    for (size_t y = 0; y < MAX_HEIGHT - 1; ++y)  {
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
    }
}

static inline void work_cycle(cmd_ctx_t* ctx) {
    uint8_t repeat_cnt = 0;
    for(;;) {
        char c = getch_now();
        if (c) {
            if (c == CHAR_CODE_BS) cmd_backspace();
            else if (c == CHAR_CODE_UP) handle_up_pressed();
            else if (c == CHAR_CODE_DOWN) handle_down_pressed();
            else if (c == CHAR_CODE_TAB) handle_tab_pressed();
            else if (c == CHAR_CODE_ENTER) enter_pressed();
            else if (c == CHAR_CODE_ESC) {
                mark_to_exit(9);
                scan_code_processed();
            }
            else if (ctrlPressed && (c == 'o' || c == 'O' || c == 0x99 /*Щ*/ || c == 0xE9 /*щ*/)) hide_pannels();
            else push_char(c);
            scan_code_processed();
            continue;
        }
        if (delPressed) {
            delPressed = false;
            cmd_del();
            scan_code_processed();
            continue;
        }
        if (rightPressed) {
            rightPressed = false;
            handle_right_pressed();
            scan_code_processed();
            continue;
        }
        if (leftPressed) {
            leftPressed = false;
            handle_left_pressed();
            scan_code_processed();
            continue;
        }
        if (homePressed) {
            homePressed = false;
            handle_home_pressed();
            scan_code_processed();
            continue;
        }
        if (endPressed) {
            endPressed = false;
            handle_end_pressed();
            scan_code_processed();
            continue;
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
                } else if (nespad_state & DPAD_START) {
                    enter_pressed();
                } else if ((nespad_state & DPAD_A) && (nespad_state & DPAD_START)) {
//                    turn_usb_on(0);
                } else if ((nespad_state & DPAD_B) && (nespad_state & DPAD_SELECT)) {
                    mark_to_exit(0);
                } else if ((nespad_state & DPAD_LEFT) || (nespad_state & DPAD_RIGHT)) {
                    handle_tab_pressed();
                } else if ((nespad_state & DPAD_A) && (nespad_state & DPAD_SELECT)) {
//                    conf_it(0);
                } else if ((nespad_state & DPAD_B) && (nespad_state & DPAD_START)) {
                    // reset?
                    return;
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
        switch(lastCleanableScanCode) {
          case 0x01: // Esc down
            mark_to_exit(9);
            scan_code_processed();
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
          case 0x49: // pageup arr down
            handle_pageup_pressed();
            scan_code_processed();
            break;
          case 0xC9: // pageup arr up
            break;
          case 0x51: // pagedown arr down
            handle_pagedown_pressed();
            scan_code_processed();
            break;
          case 0xD1: // pagedown arr up
            break;
          default:
            break;
        }
        if(mark_to_exit_flag) {
            restore_console(ctx);
            return;
        }
    }
}

static size_t string_size_bytes(string_t* s) {
    return s ? s->size + 1 : 0;
}

inline static void start_editor(cmd_ctx_t* ctx) {
    lst = new_list_v(new_string_v, delete_string, string_size_bytes); // list of string_t*
    f_sz = 0;
    {
        FILINFO* fno = malloc(sizeof(FILINFO));
        if (FR_OK != f_stat(ctx->argv[1], fno) || (fno->fattrib & AM_DIR)) {
            free(fno);
            list_push_back(lst, new_string_v());
            goto nw; // assumed new file creation
        }
        f_sz = fno->fsize;
        free(fno);
    }
    size_t free_sz = xPortGetFreeHeapSize();
    if (f_sz * 2 > free_sz) { // TODO: virtual RAM
        char line[32];
        snprintf(line, 16, "File size: %d", f_sz);
        const line_t lns[2] = {
            { -1, "Not enough SRAM for this operation" },
            { -1, line }
        };
        const lines_t lines = { 2, 3, lns };
        draw_box((MAX_WIDTH - 60) / 2, 7, 60, 10, "Error", &lines);
        vTaskDelay(1500);
        return false;
    }
    char* buff;
    {
        FIL* f = malloc(sizeof(FIL));
        if (FR_OK != f_open(f, ctx->argv[1], FA_READ)) {
            free(f);
            delete_list(lst);
            return false;
        }
        buff = malloc(f_sz); // TODO: dynamic
        UINT br;
        if (FR_OK != f_read(f, buff, f_sz, &br) || br != f_sz) {
            free(buff);
            free(f);
            delete_list(lst);
            return false;
        }
        f_close(f);
        free(f);
    }
    string_t* s = new_string_v();
    for (size_t i = 0; i < f_sz; ++i) {
        char c = buff[i];
        if (c == '\r') continue;
        if (c == '\n') {
            list_push_back(lst, s);
            s = new_string_v();
        } else {
            string_push_back_c(s, c);
        }
    }
    free(buff);
nw:
    m_window();
    bottom_line();

    work_cycle(ctx);

// TODO: ask for save?
    delete_list(lst);
}

int main(void) {
    cmd_ctx_t* ctx = get_cmd_ctx();
    if (ctx->argc == 0) {
        fprintf(ctx->std_err, "ATTTENTION! BROKEN EXECUTION CONTEXT [%p]!\n", ctx);
        return -1;
    }
    if (ctx->argc != 2) {
        fprintf(ctx->std_err, "Unexpected number of arguemts: %d\n", ctx->argc);
        return 1;
    }
    MAX_WIDTH = get_console_width();
    MAX_HEIGHT = get_console_height();
    F_BTN_Y_POS = TOTAL_SCREEN_LINES - 1;
    PANEL_LAST_Y = F_BTN_Y_POS - 1;
    LAST_FILE_LINE_ON_PANEL_Y = PANEL_LAST_Y - 1;
    save_console(ctx);

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

    scancode_handler = get_scancode_handler();
    set_scancode_handler(scancode_handler_impl);

    set_cursor_color(15); // TODO: blinking
    start_editor(ctx);
    set_cursor_color(7); // TODO: rc/config?

    set_scancode_handler(scancode_handler);
    free(pcs);

    return 0;
}

int __required_m_api_verion(void) {
    return M_API_VERSION;
}
