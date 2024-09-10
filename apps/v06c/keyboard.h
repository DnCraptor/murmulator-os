#pragma once

enum
{
    SCANCODE_SPACE,
    SCANCODE_GRAVE,
    SCANCODE_RIGHTBRACKET,
    SCANCODE_BACKSLASH,
    SCANCODE_LEFTBRACKET,
    SCANCODE_Z,
    SCANCODE_Y,
    SCANCODE_X,

    SCANCODE_W,
    SCANCODE_V,
    SCANCODE_U,
    SCANCODE_T,
    SCANCODE_S,
    SCANCODE_R,
    SCANCODE_Q,
    SCANCODE_P,

    SCANCODE_O,
    SCANCODE_N,
    SCANCODE_M,
    SCANCODE_L,
    SCANCODE_K,
    SCANCODE_J,
    SCANCODE_I,
    SCANCODE_H,

    SCANCODE_G,
    SCANCODE_F,
    SCANCODE_E,
    SCANCODE_D,
    SCANCODE_C,
    SCANCODE_B,
    SCANCODE_A,
    SCANCODE_AT,

    SCANCODE_SLASH,
    SCANCODE_PERIOD,
    SCANCODE_MINUS,
    SCANCODE_COMMA,
    SCANCODE_SEMICOLON,
    SCANCODE_APOSTROPHE,
    SCANCODE_9,
    SCANCODE_8,

    SCANCODE_7,
    SCANCODE_6,
    SCANCODE_5,
    SCANCODE_4,
    SCANCODE_3,
    SCANCODE_2,
    SCANCODE_1,
    SCANCODE_0,

    SCANCODE_F5,
    SCANCODE_F4,
    SCANCODE_F3,
    SCANCODE_F2,
    SCANCODE_F1,
    SCANCODE_AR2,
    SCANCODE_STR,
    SCANCODE_HOME,
    SCANCODE_DOWN,
    SCANCODE_RIGHT,
    SCANCODE_UP,
    SCANCODE_LEFT,
    SCANCODE_BACKSPACE,
    SCANCODE_RETURN,
    SCANCODE_PS,
    SCANCODE_TAB,

    SCANCODE_RUSLAT,
    SCANCODE_US,
    SCANCODE_SS,
    SCANCODE_SBROS,
    SCANCODE_VVOD
};

namespace keyboard
{

// mod keys are active low
constexpr uint8_t BLK_BIT_VVOD =     (1 << 0);
constexpr uint8_t BLK_BIT_SBROS =    (1 << 1);
constexpr uint8_t BLK_MASK = BLK_BIT_VVOD | BLK_BIT_SBROS;

constexpr uint8_t PC_BIT_SS =       (1<<5);
constexpr uint8_t PC_BIT_US =       (1<<6);
constexpr uint8_t PC_BIT_RUSLAT =   (1<<7);
// only the modkeys that go to port C
constexpr uint8_t PC_MODKEYS_MASK = PC_BIT_SS | PC_BIT_US | PC_BIT_RUSLAT;

constexpr uint8_t PC_BIT_INDRUS =   (1<<3);

struct keyboard_state_t
{
    uint8_t rows;           // rows input
    uint8_t pc;             // modkeys input
    uint8_t blk;            // vvod/sbros
    uint8_t ruslat;         // output 
};

extern keyboard_state_t state;
extern keyboard_state_t io_state;
extern bool osd_enable;

extern uint8_t rows[9];

typedef void (*onkeyevent_t)(int,int,bool);
//extern std::function<void(int,int,bool)> onkeyevent; // scancode, char code, 1 = make, 0 = break
extern onkeyevent_t onkeyevent; // scancode, char code, 1 = make, 0 = break

void io_select_columns(uint8_t pa);
void io_read_rows();
void io_read_modkeys();
void io_commit_ruslat();
void io_out_ruslat(uint8_t w8);

// when osd takes over keyboard control, block all io_*() calls
void osd_takeover(bool enable, bool nowait = false);
void scan_matrix();
void detect_changes();

void select_columns(uint8_t pa); // out 03 (PA) and forget
void read_rows();  // finalize SPI transaction and update state
void read_modkeys();
void out_ruslat(uint8_t w8);
void commit_ruslat();
void init(void);

bool vvod_pressed();
bool sbros_pressed();

}
