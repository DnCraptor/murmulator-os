#include "m-os-api.h"
#include "m-os-api-sdtfn.h"

const char TEMP[] = "TEMP";
const char _mc_con[] = ".mc.con";

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

static volatile bool ctrlPressed = false;
static volatile bool altPressed = false;
static volatile bool delPressed = false;
static volatile bool leftPressed = false;
static volatile bool rightPressed = false;
static volatile bool upPressed = false;
static volatile bool downPressed = false;

static size_t line_s = 0;
static size_t line_e = 0;

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

static color_schema_t* pcs;

int _init(void) {
    hidePannels = false;

    ctrlPressed = false;
    altPressed = false;
    delPressed = false;
    leftPressed = false;
    rightPressed = false;
    upPressed = false;
    downPressed = false;

    marked_to_exit = false;
    line_s = 0;
    line_e = 0;
    scan_code_cleanup();
}

static void do_nothing(uint8_t cmd) {
    char line[32];
    snprintf(line, MAX_WIDTH, "CMD: F%d", cmd + 1);
    const line_t lns[2] = {
        { -1, "Not yet implemented function" },
        { -1, line }
    };
    const lines_t lines = { 2, 3, lns };
    draw_box(pcs, (MAX_WIDTH - 60) / 2, 7, 60, 10, "Info", &lines);
    vTaskDelay(1500);
    redraw_window();
}

static bool m_prompt(const char* txt) {
    const line_t lns[1] = {
        { -1, txt },
    };
    const lines_t lines = { 1, 2, lns };
    draw_box(pcs, (MAX_WIDTH - 60) / 2, 7, 60, 10, "Are you sure?", &lines);
    bool yes = true;
    draw_button(pcs, (MAX_WIDTH - 60) / 2 + 16, 12, 11, "Yes", yes);
    draw_button(pcs, (MAX_WIDTH - 60) / 2 + 35, 12, 10, "No", !yes);
    while(1) {
        char c = getch_now();
        if (c == CHAR_CODE_ENTER) {
            scan_code_cleanup();
            return yes;
        }
        if (c == CHAR_CODE_TAB || leftPressed || rightPressed) { // TODO: own msgs cycle
            yes = !yes;
            draw_button(pcs, (MAX_WIDTH - 60) / 2 + 16, 12, 11, "Yes", yes);
            draw_button(pcs, (MAX_WIDTH - 60) / 2 + 35, 12, 10, "No", !yes);
            leftPressed = rightPressed = false;
            scan_code_cleanup();
        }
        if (c == CHAR_CODE_ESC) {
            scan_code_cleanup();
            return false;
        }
    }
}

static void mark_to_exit(uint8_t cmd) {
    marked_to_exit = true;
}

static void m_info(uint8_t cmd) {
    line_t plns[2] = {
        { 1, " It is Murmulator Viewer" },
        { 1, " Esc or F10 to Exit" }
    };
    lines_t lines = { 2, 0, plns };
    draw_box(pcs, 5, 2, MAX_WIDTH - 15, MAX_HEIGHT - 6, "Help", &lines);
    char c = 0;
    while(c != CHAR_CODE_ENTER && c != CHAR_CODE_ESC) {
        c = getch_now();
    }
    redraw_window();
}

static fn_1_12_tbl_t fn_1_12_tbl = {
    ' ', '1', " Info ", m_info,
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
    cmd_ctx_t* ctx = get_cmd_ctx();
    draw_panel(pcs, 0, PANEL_TOP_Y, MAX_WIDTH, PANEL_LAST_Y + 1, ctx->argv[1], 0);
    FIL* f = malloc(sizeof(FIL));
    if (FR_OK != f_open(f, ctx->argv[1], FA_READ)) {
        const line_t lns[2] = {
            { -1, "Unable to open file:" },
            { -1, ctx->argv[1] }
        };
        const lines_t lines = { 2, 3, lns };
        draw_box(pcs, (MAX_WIDTH - 60) / 2, 7, 60, 10, "Error", &lines);
        vTaskDelay(1500);
        goto e1;
    }
    size_t width = MAX_WIDTH - 2;
    size_t height = MAX_HEIGHT - 3;
    char* buff = malloc(width);
    size_t y = 1;
    size_t line = 0;
    while (f_read_str(f, buff, width)) {
        if (line >= line_s && y <= height) {
            draw_text(buff, 1, y, pcs->FOREGROUND_FIELD_COLOR, pcs->BACKGROUND_FIELD_COLOR);
            ++y;
            line_e = line;
        }
        ++line;
    }

    size_t free_sz = xPortGetFreeHeapSize();
    snprintf(buff, width, " Lines: %d-%d of %d", (line_s + 1), (line_e + 1), line);
    draw_text(
        buff,
        2,
        PANEL_LAST_Y,
        pcs->FOREGROUND_FIELD_COLOR,
        pcs->BACKGROUND_FIELD_COLOR
    );
e2:
    free(buff);
    f_close(f);
e1:
    free(f);
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
    uint8_t c = (uint8_t)ps2scancode & 0xFF;
    if (c == 0x4B) {
        leftPressed = true;
    } else if (c == 0xCB) {
        leftPressed = false;
    } else if (c == 0x4D) {
        rightPressed = true;
    } else if (c == 0xCD) {
        rightPressed = false;
    } else if (c == 0x38) {
        altPressed = true;
    } else if (c == 0xB8) {
        altPressed = false;
    } else if (c == 0x1D) {
        ctrlPressed = true;
    } else if (c == 0x9D) {
        ctrlPressed = false;
    } else if (c == 0x53) {
        delPressed = true;
    } else if (c == 0xD3) {
        delPressed = false;
    }
r:
    if (scancode_handler) {
        return scancode_handler(ps2scancode);
    }
    return false;
}

static inline void redraw_current_panel() {
    m_window();
}

static void enter_pressed() {
    if (hidePannels) return;
    ++line_s;
    m_window();
}

inline static void handle_pagedown_pressed() {
    if (hidePannels) return;
    line_s += MAX_HEIGHT - 4;
    m_window();
}

inline static void handle_down_pressed() {
    if (hidePannels) return;
    ++line_s;
    m_window();
}

inline static void handle_pageup_pressed() {
    if (hidePannels) return;
    if (line_s <= MAX_HEIGHT - 4) {
        line_s = 0;
    } else {
        line_s -= MAX_HEIGHT - 4;
    }
    m_window();
}

inline static void handle_up_pressed() {
    if (hidePannels) return;
    if (line_s >= 1) {
        --line_s;
        m_window();
    }
}

inline static fn_1_12_btn_pressed(uint8_t fn_idx) {
    if (fn_idx > 11) fn_idx -= 18; // F11-12
    (*actual_fn_1_12_tbl())[fn_idx].action(fn_idx);
}

inline static void cmd_backspace() {
}

inline static void handle_tab_pressed() {
    if (hidePannels) {
        return;
    }
}

inline static void restore_console(cmd_ctx_t* ctx) {
    op_console(ctx, f_read, FA_READ);
}

inline static void save_console(cmd_ctx_t* ctx) {
    op_console(ctx, f_write, FA_CREATE_ALWAYS | FA_WRITE);
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
            else if (c == CHAR_CODE_ESC) mark_to_exit(9);
            else if (ctrlPressed && (c == 'o' || c == 'O' || c == 0x99 /*Щ*/ || c == 0xE9 /*щ*/)) hide_pannels();
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
        uint8_t sc = lastCleanableScanCode;
        if ((sc >= 0x3B && sc <= 0x44) || sc == 0x57 || sc == 0x58 || sc == 0x49 || sc == 0x51 || sc == 0x1C || sc == 0x9C) { // F1..12 down,/..
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
                scan_code_processed();
            }
        } else if (sc == 0xD1) {
            if (lastSavedScanCode == 0x51) {
                handle_pagedown_pressed();
                scan_code_processed();
            }
        }
        if(marked_to_exit) {
            restore_console(ctx);
            return;
        }
    }
}

inline static void start_viewer(cmd_ctx_t* ctx) {
    m_window();
    bottom_line();
    work_cycle(ctx);
}

int main(void) {
    cmd_ctx_t* ctx = get_cmd_ctx();
    if (ctx->argc > 2) {
        fprintf(ctx->std_err, "Unexpected number of arguemts: %d\n", ctx->argc);
        return 1;
    }
    graphics_set_con_pos(-1, 0);
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

    start_viewer(ctx);

    set_scancode_handler(scancode_handler);
    free(pcs);
    graphics_set_con_pos(0, PANEL_LAST_Y);

    return 0;
}
