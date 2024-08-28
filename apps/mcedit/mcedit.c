#include "m-os-api.h"

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
static volatile bool homePressed = false;
static volatile bool endPressed = false;

static size_t line_s = 0;
static size_t col_s = 0;
static size_t line_n = 0;
static size_t col_n = 0;
static size_t f_sz;
static list_t* lst;

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

    ctrlPressed = false;
    altPressed = false;
    delPressed = false;
    leftPressed = false;
    rightPressed = false;
    upPressed = false;
    downPressed = false;
    homePressed = false;
    endPressed = false;

    marked_to_exit = false;
    line_s = 0;
    col_s = 0;
    line_n = 0;
    col_n = 0;
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
    size_t width = MAX_WIDTH > 60 ? 60 : 40;
    size_t shift = MAX_WIDTH > 60 ? 10 : 0;
    size_t x = (MAX_WIDTH - width) >> 1;
    draw_box(pcs, (MAX_WIDTH - 60) / 2, 7, 60, 10, "Are you sure?", &lines);
    bool yes = true;
    draw_button(pcs, x + shift + 6, 12, 11, "Yes", yes);
    draw_button(pcs, x + shift + 25, 12, 10, "No", !yes);
    while(1) {
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
        }
        if (c == CHAR_CODE_TAB || leftPressed || rightPressed) { // TODO: own msgs cycle
            yes = !yes;
            draw_button(pcs, x + shift + 6, 12, 11, "Yes", yes);
            draw_button(pcs, x + shift + 25, 12, 10, "No", !yes);
            leftPressed = rightPressed = false;
            scan_code_cleanup();
        }
    }
}

static void do_mark_to_exit(uint8_t cmd) {
    marked_to_exit = true;
}

static void m_info(uint8_t cmd) {
    line_t plns[2] = {
        { 1, " It is ZX Murmulator OS Commander Editor" },
        { 1, " Let edit this file and do not miss to save it using F2 button." }
    };
    lines_t lines = { 2, 0, plns };
    draw_box(pcs, 5, 2, MAX_WIDTH - 15, MAX_HEIGHT - 6, "Help", &lines);
    char c;
    do {
        c = getch();
    } while(c != CHAR_CODE_ESC && c != CHAR_CODE_ENTER);
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
    '1', '0', " Exit ", do_mark_to_exit,
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
    '1', '0', " Exit ", do_mark_to_exit,
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
    '1', '0', " Exit ", do_mark_to_exit,
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
    draw_panel(pcs, 0, PANEL_TOP_Y, MAX_WIDTH, PANEL_LAST_Y + 1, ctx->argv[1], 0);

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
    } else if (sc == 0x53) {
        delPressed = true;
    } else if (sc == 0xD3) {
        delPressed = false;
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

static void handle_end_pressed(void);

inline static void handle_left_pressed(void) {
    if (hidePannels || col_n == 0) {
        if (line_n != 0) {
            --line_n;
            handle_end_pressed();
        }
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

static void handle_end_pressed(void) {
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
            else if (c == CHAR_CODE_ESC) {
                marked_to_exit = true;
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
        draw_box(pcs, (MAX_WIDTH - 60) / 2, 7, 60, 10, "Error", &lines);
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
