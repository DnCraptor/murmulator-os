#include "hardware/pio.h"

#define nespad_wrap_target 0
#define nespad_wrap 6

static const uint16_t nespad_program_instructions[] = {
    //     .wrap_target
    0x80a0, //  0: pull   block  
    0xea01, //  1: set    pins, 1         side 0 [10]
    0xe027, //  2: set    x, 7            side 0
    0xe000, //  3: set    pins, 0         side 0
    0x4402, //  4: in     pins, 2         side 0 [4]      <--- 2
    0xf500, //  5: set    pins, 0         side 1 [5]
    0x0044, //  6: jmp    x--, 4          side 0
            //     .wrap
};

static const struct pio_program nespad_program = {
    .instructions = nespad_program_instructions,
    .length =  7,
    .origin = -1,
};

static inline pio_sm_config nespad_program_get_default_config(uint offset) {
  pio_sm_config c = pio_get_default_sm_config();
  sm_config_set_wrap(&c, offset + nespad_wrap_target, offset + nespad_wrap);
  sm_config_set_sideset(&c, 1, false, false);
  return c;
}

static PIO pio = pio1;
static uint8_t sm = -1;
uint8_t nespad_state  = 0;  // Joystick 1
uint8_t nespad_state2 = 0;  // Joystick 2

bool nespad_begin(uint32_t cpu_khz, uint8_t clkPin, uint8_t dataPin,uint8_t latPin) {
  if (pio_can_add_program(pio, &nespad_program) &&
      ((sm = pio_claim_unused_sm(pio, true)) >= 0)) {
    uint offset = pio_add_program(pio, &nespad_program);
    pio_sm_config c = nespad_program_get_default_config(offset);

    sm_config_set_sideset_pins(&c, clkPin);
    sm_config_set_in_pins(&c, dataPin);
    sm_config_set_set_pins(&c, latPin, 1);
    pio_gpio_init(pio, clkPin);
    pio_gpio_init(pio, dataPin);
    pio_gpio_init(pio, dataPin+1);  // +1 Pin for Joystick2
    pio_gpio_init(pio, latPin);
    gpio_set_pulls(dataPin, true, false); // Pull data high, 0xFF if unplugged
    gpio_set_pulls(dataPin+1, true, false); // Pull data high, 0xFF if unplugged for Joystick2

    pio_sm_set_pindirs_with_mask(pio, sm,
                                 (1 << clkPin) | (1 << latPin), // Outputs
                                 (1 << clkPin) | (1 << latPin) | 
                                 (1 << dataPin) | (1 << (dataPin+1))
                                ); // All pins
    sm_config_set_in_shift(&c, true, true, 16); // R shift, autopush @ 8 bits (@ 16 bits for 2 Joystick)

    sm_config_set_clkdiv_int_frac(&c, cpu_khz / 1000, 0); // 1 MHz clock



    pio_sm_clear_fifos(pio, sm);

    pio_sm_init(pio, sm, offset, &c);
    pio_sm_set_enabled(pio, sm, true);
    pio->txf[sm]=0;
    return true; // Success
  }
  return false;
}



// nespad read. Ideally should be called ~100 uS after
// nespad_read_start(), but can be sooner (will block until ready), or later
// (will introduce latency). Sets value of global nespad_state variable, a
// bitmask of button/D-pad state (1 = pressed). 0x80=Right, 0x40=Left,
// 0x20=Down, 0x10=Up, 0x08=Start, 0x04=Select, 0x02=B, 0x01=A. Must first
// call nespad_begin() once to set up PIO. Result will be 0 if PIO failed to
// init (e.g. no free state machine).

void nespad_read()
{
  if (sm<0) return;
  if (pio_sm_is_rx_fifo_empty(pio, sm)) return;

  // Right-shift was used in sm config so bit order matches NES controller
  // bits used elsewhere in picones, but does require shifting down...
  uint16_t temp16=((pio->rxf[sm])>>16)^ 0xFFFF;
  pio->txf[sm]=0;
  uint16_t temp1, temp2;
  // temp1  = temp16 & 0x5555;             // 08070605.04030201
  // temp2  = temp16 & 0xAAAA;             // 80706050.40302010
  // nespad_state=temp1|(temp1>>7); //84736251
  //nespad_state2=(temp2|(temp2<<7))>>8;//84736251
  // return;


// 1 -------------------------------------------------------  
  temp1  = temp16 & 0x5555;             // 08070605.04030201
  temp2  = temp16 & 0xAAAA;             // 80706050.40302010
  temp16 = temp16 & 0xAA55;             // 80706050.04030201
  temp1  = temp1 >> 7;                  // 00000000.80706050
  temp2  = temp2 << 7;                  // 04030201.00000000
  temp16 = temp16 | temp1 | temp2;      // 84736251.84736251
// 2 -------------------------------------------------------
  temp1  = temp16 & 0x5050;             // 04030000.04030000
  temp2  = temp16 & 0x0A0A;             // 00006050.00006050
  temp16 = temp16 & 0xA5A5;             // 80700201.80700201
  temp1  = temp1 >> 3;                  // 00004030.00004030
  temp2  = temp2 << 3;                  // 06050000.06050000
  temp16 = temp16 | temp1 | temp2;      // 86754231.86754231
// 3 -------------------------------------------------------
  temp1  = temp16 & 0x4444;             // 06000200.06000200
  temp2  = temp16 & 0x2222;             // 00700030.00700030
  temp16 = temp16 & 0x9999;             // 80054001.80054001
  temp1  = temp1 >> 1;                  // 00600020.00600020
  temp2  = temp2 << 1;                  // 07000300.07000300
  temp16 = temp16 | temp1 | temp2;      // 87654321.87654321
//----------------------------------------------------------
  nespad_state  = temp16;               // 00000000.87654321 Joy1
  nespad_state2 = temp16 >> 8;          // 00000000.87654321 Joy2
}


