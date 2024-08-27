/*
 *
 * $Id: hardware.h,v 1.1 2024/02/25 04:43:16 stefan Exp stefan $
 *
 * Stefan's basic interpreter 
 *
 * See the licence file on 
 * https://github.com/slviajero/tinybasic for copyright/left.
 *   (GNU GENERAL PUBLIC LICENSE, Version 3, 29 June 2007)
 *
 * Author: Stefan Lenz, sl001@serverfabrik.de
 * 
 *
 * Runtime environment for the BASIC interpreter, the 
 * hardware configuration file. It contains all hardware 
 * related settings and dependencies.
 *
 * This is hardware.h for POSIX systems and the POSIX runtime
 * environment runtime.c. 
 * 
 * Default setting is POSIX for most UNIXes including Mac.
 * RASPPI activates Raspberry PI related features. 
 * MINGW activates windows related features.
 * MSDOS for MSDOS compiles with tc 2.0 (untested in this version, last tested in 1.4)
 *
 */
#define MURMULATOR
///#include "boards/pico.h"
#ifdef MURMULATOR

#ifndef LED_BUILTIN
#define LED_BUILTIN 25

#define _u(x) x ## u
#define SIO_BASE _u(0xd0000000)
#define SIO_CPUID_OFFSET _u(0x00000000)
#define SIO_GPIO_IN_OFFSET _u(0x00000004)
#define SIO_GPIO_HI_IN_OFFSET _u(0x00000008)
#define SIO_GPIO_OUT_OFFSET _u(0x00000010)
#define SIO_GPIO_OUT_SET_OFFSET _u(0x00000014)
#define SIO_GPIO_OUT_CLR_OFFSET _u(0x00000018)
#define SIO_GPIO_OUT_XOR_OFFSET _u(0x0000001c)
#define SIO_GPIO_OE_OFFSET _u(0x00000020)
#define SIO_GPIO_OE_SET_OFFSET _u(0x00000024)
#define SIO_GPIO_OE_CLR_OFFSET _u(0x00000028)
#define SIO_GPIO_OE_XOR_OFFSET _u(0x0000002c)
#define SIO_GPIO_HI_OUT_OFFSET _u(0x00000030)
#define SIO_GPIO_HI_OUT_SET_OFFSET _u(0x00000034)
#define SIO_GPIO_HI_OUT_CLR_OFFSET _u(0x00000038)
#define SIO_GPIO_HI_OUT_XOR_OFFSET _u(0x0000003c)
#define SIO_GPIO_HI_OE_OFFSET _u(0x00000040)
#define SIO_GPIO_HI_OE_SET_OFFSET _u(0x00000044)
#define SIO_GPIO_HI_OE_CLR_OFFSET _u(0x00000048)
#define SIO_GPIO_HI_OE_XOR_OFFSET _u(0x0000004c)
#define SIO_FIFO_ST_OFFSET _u(0x00000050)
#define SIO_FIFO_WR_OFFSET _u(0x00000054)
#define SIO_FIFO_RD_OFFSET _u(0x00000058)
#define SIO_SPINLOCK_ST_OFFSET _u(0x0000005c)
#define SIO_DIV_UDIVIDEND_OFFSET _u(0x00000060)
#define SIO_DIV_UDIVISOR_OFFSET _u(0x00000064)
#define SIO_DIV_SDIVIDEND_OFFSET _u(0x00000068)
#define SIO_DIV_SDIVISOR_OFFSET _u(0x0000006c)
#define SIO_DIV_QUOTIENT_OFFSET _u(0x00000070)
#define SIO_DIV_REMAINDER_OFFSET _u(0x00000074)
#define SIO_DIV_CSR_OFFSET _u(0x00000078)
#define SIO_INTERP0_ACCUM0_OFFSET _u(0x00000080)
#define SIO_INTERP0_ACCUM1_OFFSET _u(0x00000084)
#define SIO_INTERP0_BASE0_OFFSET _u(0x00000088)
#define SIO_INTERP0_POP_LANE0_OFFSET _u(0x00000094)
#define SIO_INTERP0_PEEK_LANE0_OFFSET _u(0x000000a0)
#define SIO_INTERP0_CTRL_LANE0_OFFSET _u(0x000000ac)
#define SIO_INTERP0_ACCUM0_ADD_OFFSET _u(0x000000b4)
#define SIO_INTERP0_BASE_1AND0_OFFSET _u(0x000000bc)
#define _REG_(x)
typedef volatile uint32_t io_rw_32;
typedef const volatile uint32_t io_ro_32;
typedef volatile uint32_t io_wo_32;
typedef struct {
    _REG_(SIO_INTERP0_ACCUM0_OFFSET) // SIO_INTERP0_ACCUM0
    // (Description copied from array index 0 register SIO_INTERP0_ACCUM0 applies similarly to other array indexes)
    //
    // Read/write access to accumulator 0
    io_rw_32 accum[2];

    _REG_(SIO_INTERP0_BASE0_OFFSET) // SIO_INTERP0_BASE0
    // (Description copied from array index 0 register SIO_INTERP0_BASE0 applies similarly to other array indexes)
    //
    // Read/write access to BASE0 register
    io_rw_32 base[3];

    _REG_(SIO_INTERP0_POP_LANE0_OFFSET) // SIO_INTERP0_POP_LANE0
    // (Description copied from array index 0 register SIO_INTERP0_POP_LANE0 applies similarly to other array indexes)
    //
    // Read LANE0 result, and simultaneously write lane results to both accumulators (POP)
    io_ro_32 pop[3];

    _REG_(SIO_INTERP0_PEEK_LANE0_OFFSET) // SIO_INTERP0_PEEK_LANE0
    // (Description copied from array index 0 register SIO_INTERP0_PEEK_LANE0 applies similarly to other array indexes)
    //
    // Read LANE0 result, without altering any internal state (PEEK)
    io_ro_32 peek[3];

    _REG_(SIO_INTERP0_CTRL_LANE0_OFFSET) // SIO_INTERP0_CTRL_LANE0
    // (Description copied from array index 0 register SIO_INTERP0_CTRL_LANE0 applies similarly to other array indexes)
    //
    // Control register for lane 0
    // 0x02000000 [25]    : OVERF (0): Set if either OVERF0 or OVERF1 is set
    // 0x01000000 [24]    : OVERF1 (0): Indicates if any masked-off MSBs in ACCUM1 are set
    // 0x00800000 [23]    : OVERF0 (0): Indicates if any masked-off MSBs in ACCUM0 are set
    // 0x00200000 [21]    : BLEND (0): Only present on INTERP0 on each core
    // 0x00180000 [20:19] : FORCE_MSB (0): ORed into bits 29:28 of the lane result presented to the processor on the bus
    // 0x00040000 [18]    : ADD_RAW (0): If 1, mask + shift is bypassed for LANE0 result
    // 0x00020000 [17]    : CROSS_RESULT (0): If 1, feed the opposite lane's result into this lane's accumulator on POP
    // 0x00010000 [16]    : CROSS_INPUT (0): If 1, feed the opposite lane's accumulator into this lane's shift + mask hardware
    // 0x00008000 [15]    : SIGNED (0): If SIGNED is set, the shifted and masked accumulator value is sign-extended to 32 bits
    // 0x00007c00 [14:10] : MASK_MSB (0): The most-significant bit allowed to pass by the mask (inclusive)
    // 0x000003e0 [9:5]   : MASK_LSB (0): The least-significant bit allowed to pass by the mask (inclusive)
    // 0x0000001f [4:0]   : SHIFT (0): Logical right-shift applied to accumulator before masking
    io_rw_32 ctrl[2];

    _REG_(SIO_INTERP0_ACCUM0_ADD_OFFSET) // SIO_INTERP0_ACCUM0_ADD
    // (Description copied from array index 0 register SIO_INTERP0_ACCUM0_ADD applies similarly to other array indexes)
    //
    // Values written here are atomically added to ACCUM0
    // 0x00ffffff [23:0]  : INTERP0_ACCUM0_ADD (0)
    io_rw_32 add_raw[2];

    _REG_(SIO_INTERP0_BASE_1AND0_OFFSET) // SIO_INTERP0_BASE_1AND0
    // On write, the lower 16 bits go to BASE0, upper bits to BASE1 simultaneously
    io_wo_32 base01;
} interp_hw_t;
// Reference to datasheet: https://datasheets.raspberrypi.com/rp2040/rp2040-datasheet.pdf#tab-registerlist_sio
//
// The _REG_ macro is intended to help make the register navigable in your IDE (for example, using the "Go to Definition" feature)
// _REG_(x) will link to the corresponding register in hardware/regs/sio.h.
//
// Bit-field descriptions are of the form:
// BITMASK [BITRANGE]: FIELDNAME (RESETVALUE): DESCRIPTION
typedef struct {
    _REG_(SIO_CPUID_OFFSET) // SIO_CPUID
    // Processor core identifier
    io_ro_32 cpuid;

    _REG_(SIO_GPIO_IN_OFFSET) // SIO_GPIO_IN
    // Input value for GPIO pins
    // 0x3fffffff [29:0]  : GPIO_IN (0): Input value for GPIO0
    io_ro_32 gpio_in;

    _REG_(SIO_GPIO_HI_IN_OFFSET) // SIO_GPIO_HI_IN
    // Input value for QSPI pins
    // 0x0000003f [5:0]   : GPIO_HI_IN (0): Input value on QSPI IO in order 0
    io_ro_32 gpio_hi_in;

    uint32_t _pad0;

    _REG_(SIO_GPIO_OUT_OFFSET) // SIO_GPIO_OUT
    // GPIO output value
    // 0x3fffffff [29:0]  : GPIO_OUT (0): Set output level (1/0 -> high/low) for GPIO0
    io_rw_32 gpio_out;

    _REG_(SIO_GPIO_OUT_SET_OFFSET) // SIO_GPIO_OUT_SET
    // GPIO output value set
    // 0x3fffffff [29:0]  : GPIO_OUT_SET (0): Perform an atomic bit-set on GPIO_OUT, i
    io_wo_32 gpio_set;

    _REG_(SIO_GPIO_OUT_CLR_OFFSET) // SIO_GPIO_OUT_CLR
    // GPIO output value clear
    // 0x3fffffff [29:0]  : GPIO_OUT_CLR (0): Perform an atomic bit-clear on GPIO_OUT, i
    io_wo_32 gpio_clr;

    _REG_(SIO_GPIO_OUT_XOR_OFFSET) // SIO_GPIO_OUT_XOR
    // GPIO output value XOR
    // 0x3fffffff [29:0]  : GPIO_OUT_XOR (0): Perform an atomic bitwise XOR on GPIO_OUT, i
    io_wo_32 gpio_togl;

    _REG_(SIO_GPIO_OE_OFFSET) // SIO_GPIO_OE
    // GPIO output enable
    // 0x3fffffff [29:0]  : GPIO_OE (0): Set output enable (1/0 -> output/input) for GPIO0
    io_rw_32 gpio_oe;

    _REG_(SIO_GPIO_OE_SET_OFFSET) // SIO_GPIO_OE_SET
    // GPIO output enable set
    // 0x3fffffff [29:0]  : GPIO_OE_SET (0): Perform an atomic bit-set on GPIO_OE, i
    io_wo_32 gpio_oe_set;

    _REG_(SIO_GPIO_OE_CLR_OFFSET) // SIO_GPIO_OE_CLR
    // GPIO output enable clear
    // 0x3fffffff [29:0]  : GPIO_OE_CLR (0): Perform an atomic bit-clear on GPIO_OE, i
    io_wo_32 gpio_oe_clr;

    _REG_(SIO_GPIO_OE_XOR_OFFSET) // SIO_GPIO_OE_XOR
    // GPIO output enable XOR
    // 0x3fffffff [29:0]  : GPIO_OE_XOR (0): Perform an atomic bitwise XOR on GPIO_OE, i
    io_wo_32 gpio_oe_togl;

    _REG_(SIO_GPIO_HI_OUT_OFFSET) // SIO_GPIO_HI_OUT
    // QSPI output value
    // 0x0000003f [5:0]   : GPIO_HI_OUT (0): Set output level (1/0 -> high/low) for QSPI IO0
    io_rw_32 gpio_hi_out;

    _REG_(SIO_GPIO_HI_OUT_SET_OFFSET) // SIO_GPIO_HI_OUT_SET
    // QSPI output value set
    // 0x0000003f [5:0]   : GPIO_HI_OUT_SET (0): Perform an atomic bit-set on GPIO_HI_OUT, i
    io_wo_32 gpio_hi_set;

    _REG_(SIO_GPIO_HI_OUT_CLR_OFFSET) // SIO_GPIO_HI_OUT_CLR
    // QSPI output value clear
    // 0x0000003f [5:0]   : GPIO_HI_OUT_CLR (0): Perform an atomic bit-clear on GPIO_HI_OUT, i
    io_wo_32 gpio_hi_clr;

    _REG_(SIO_GPIO_HI_OUT_XOR_OFFSET) // SIO_GPIO_HI_OUT_XOR
    // QSPI output value XOR
    // 0x0000003f [5:0]   : GPIO_HI_OUT_XOR (0): Perform an atomic bitwise XOR on GPIO_HI_OUT, i
    io_wo_32 gpio_hi_togl;

    _REG_(SIO_GPIO_HI_OE_OFFSET) // SIO_GPIO_HI_OE
    // QSPI output enable
    // 0x0000003f [5:0]   : GPIO_HI_OE (0): Set output enable (1/0 -> output/input) for QSPI IO0
    io_rw_32 gpio_hi_oe;

    _REG_(SIO_GPIO_HI_OE_SET_OFFSET) // SIO_GPIO_HI_OE_SET
    // QSPI output enable set
    // 0x0000003f [5:0]   : GPIO_HI_OE_SET (0): Perform an atomic bit-set on GPIO_HI_OE, i
    io_wo_32 gpio_hi_oe_set;

    _REG_(SIO_GPIO_HI_OE_CLR_OFFSET) // SIO_GPIO_HI_OE_CLR
    // QSPI output enable clear
    // 0x0000003f [5:0]   : GPIO_HI_OE_CLR (0): Perform an atomic bit-clear on GPIO_HI_OE, i
    io_wo_32 gpio_hi_oe_clr;

    _REG_(SIO_GPIO_HI_OE_XOR_OFFSET) // SIO_GPIO_HI_OE_XOR
    // QSPI output enable XOR
    // 0x0000003f [5:0]   : GPIO_HI_OE_XOR (0): Perform an atomic bitwise XOR on GPIO_HI_OE, i
    io_wo_32 gpio_hi_oe_togl;

    _REG_(SIO_FIFO_ST_OFFSET) // SIO_FIFO_ST
    // Status register for inter-core FIFOs (mailboxes)
    // 0x00000008 [3]     : ROE (0): Sticky flag indicating the RX FIFO was read when empty
    // 0x00000004 [2]     : WOF (0): Sticky flag indicating the TX FIFO was written when full
    // 0x00000002 [1]     : RDY (1): Value is 1 if this core's TX FIFO is not full (i
    // 0x00000001 [0]     : VLD (0): Value is 1 if this core's RX FIFO is not empty (i
    io_rw_32 fifo_st;

    _REG_(SIO_FIFO_WR_OFFSET) // SIO_FIFO_WR
    // Write access to this core's TX FIFO
    io_wo_32 fifo_wr;

    _REG_(SIO_FIFO_RD_OFFSET) // SIO_FIFO_RD
    // Read access to this core's RX FIFO
    io_ro_32 fifo_rd;

    _REG_(SIO_SPINLOCK_ST_OFFSET) // SIO_SPINLOCK_ST
    // Spinlock state
    io_ro_32 spinlock_st;

    _REG_(SIO_DIV_UDIVIDEND_OFFSET) // SIO_DIV_UDIVIDEND
    // Divider unsigned dividend
    io_rw_32 div_udividend;

    _REG_(SIO_DIV_UDIVISOR_OFFSET) // SIO_DIV_UDIVISOR
    // Divider unsigned divisor
    io_rw_32 div_udivisor;

    _REG_(SIO_DIV_SDIVIDEND_OFFSET) // SIO_DIV_SDIVIDEND
    // Divider signed dividend
    io_rw_32 div_sdividend;

    _REG_(SIO_DIV_SDIVISOR_OFFSET) // SIO_DIV_SDIVISOR
    // Divider signed divisor
    io_rw_32 div_sdivisor;

    _REG_(SIO_DIV_QUOTIENT_OFFSET) // SIO_DIV_QUOTIENT
    // Divider result quotient
    io_rw_32 div_quotient;

    _REG_(SIO_DIV_REMAINDER_OFFSET) // SIO_DIV_REMAINDER
    // Divider result remainder
    io_rw_32 div_remainder;

    _REG_(SIO_DIV_CSR_OFFSET) // SIO_DIV_CSR
    // Control and status register for divider
    // 0x00000002 [1]     : DIRTY (0): Changes to 1 when any register is written, and back to 0 when QUOTIENT is read
    // 0x00000001 [0]     : READY (1): Reads as 0 when a calculation is in progress, 1 otherwise
    io_ro_32 div_csr;
    uint32_t _pad1;
    interp_hw_t interp[2];
} sio_hw_t;
#define sio_hw ((sio_hw_t *)SIO_BASE)
/*! \brief Drive high every GPIO appearing in mask
 *  \ingroup hardware_gpio
 *
 * \param mask Bitmask of GPIO values to set, as bits 0-29
 */
static inline void gpio_set_mask(uint32_t mask) {
    sio_hw->gpio_set = mask;
}
/*! \brief Drive low every GPIO appearing in mask
 *  \ingroup hardware_gpio
 *
 * \param mask Bitmask of GPIO values to clear, as bits 0-29
 */
static inline void gpio_clr_mask(uint32_t mask) {
    sio_hw->gpio_clr = mask;
}
/*! \brief Drive a single GPIO high/low
 *  \ingroup hardware_gpio
 *
 * \param gpio GPIO number
 * \param value If false clear the GPIO, otherwise set it.
 */
static inline void gpio_put(unsigned gpio, bool value) {
    uint32_t mask = 1ul << gpio;
    if (value)
        gpio_set_mask(mask);
    else
        gpio_clr_mask(mask);
}
#endif
#endif

#undef MINGW
#undef MSDOS
#define RASPPI
#undef POSIX

/* 
 * Hardware flags of the POSIX systems 
 * POSIXTERMINAL, POSIXVT52TOANSI: ensure compatibility of BASIC programs  
 *  control characters of BASIC are translated to ANSI, bringing the Aruino 
 *  VT52 commands to POSIX
 * POSIXSIGNALS: enables signal handling of ^C interrupting programs
 * POSIXNONBLOCKING: non blocking I/O to handle GET and the BREAKCHAR 
 *  tricky on DOS, not very portable, experimental, use signals instead
 * POSIXFRAMEBUFFER: directly draw to the frame buffer of Raspberry PI
 *  only tested on this platform
 * POSIXWIRE: simple Raspberry PI wire code
 * POSIXMQTT: analogous to ARDUINOMQTT, send and receive MQTT messages (unfinished)
 * POSIXWIRING: use the (deprectated) wiring code for gpio on Raspberry Pi
 * POSIXPIGPIO: use the pigpio library on a Raspberry PI 
 */
#define POSIXTERMINAL
#define POSIXVT52TOANSI
#undef POSIXSIGNALS
#undef POSIXNONBLOCKING
#undef POSIXFRAMEBUFFER
#undef POSIXWIRE
#undef POSIXMQTT
#undef POSIXWIRING
#undef POSIXPIGPIO

/* simulates SPI RAM, only test code, keep undefed if you don't want to do something special */
#undef SPIRAMSIMULATOR

#ifdef SPIRAMSIMULATOR
#define USEMEMINTERFACE
#endif

/* use a serial port as printer interface - unfinished - similar to Arduino */
#undef POSIXPRT

/* use TAB in MS mode */
#define HASMSTAB

/* used pins and other parameters */

/* set this is you want pin 4 on low interrupting the interpreter */
/* #define BREAKPIN 4 */
#define BREAKPIN 4

/* the SIGNAL the interpreters listens to for interrupt */
#define BREAKSIGNAL SIGINT

/* in case of non blocking IO turn on background tasks */
#ifdef POSIXNONBLOCKING
#define BASICBGTASK
#endif

/* frame buffer health check - currently only supported on Raspberry */ 
#ifndef RASPPI
#undef POSIXFRAMEBUFFER
#endif

/* wire parameters for Raspberry*/
#define POSIXI2CBUS 1

/* the size of the EEPROM dummy */
#define EEPROMSIZE 1024

/* they all have this */
#define FILESYSTEMDRIVER

/* and they have this */
#define HASCLOCK

/* some have this */
#if defined(POSIXWIRE) && defined(POSIXPIGPIO)
#define HASWIRE
#endif

/* we can do graphics and tell language.h that we can */
#ifdef POSIXFRAMEBUFFER
#define DISPLAYHASGRAPH
#endif

#ifdef MURMULATOR
#define DISPLAYHASGRAPH
#define BUFFER_8_BIT
#define BUFFER_4_BIT
#endif

/* after run behaviour on POSIX systems, 1 to terminate if started
    on the command line with a file argument, 0 to stay active and 
    show a BASIC prompt*/
#define TERMINATEAFTERRUN 1

/* 
 * all the things the POSIX platforms need to source 
 */

/* define all the types Arduino has for compatibility of the interface */

typedef unsigned char uint8_t;
typedef signed char int8_t;
typedef unsigned short uint16_t;
typedef signed short int16_t;
typedef unsigned char byte;
typedef unsigned long long uint64_t;
///typedef unsigned int uint32_t;

/*
 * additional prototypes in an non Arduino world 
 */
inline static unsigned long millis();

/* the ususal suspects */
///#include <stdio.h>
///#include <stdlib.h>

/* floating point maths only needed if HASFLOAT*/
#include <math.h>
#include <float.h>

/* time stuff */
#include <time.h>
#include <sys/types.h>
#include <sys/timeb.h>

/* directories and files */
#ifndef MSDOS
///#include <dirent.h>
///#include <unistd.h>
#else
#include <dir.h>
#include <dos.h>
#endif

/* windowy things for windows */
#ifdef MINGW
#include <windows.h>
#endif

/* 
 *  Tell BASIC we have a second serial port
 */
#ifdef POSIXPRT
#define HASSERIAL1
#endif

/* 
 *  Tell BASIC we have a radio adapter
 */
#undef HASRF24

/* 
 *  Tell BASIC we have MQTT
 */
#ifdef POSIXMQTT
#define HASMQTT
#endif

/* we definitely don't run on an Arduino */
#undef ARDUINOPROGMEM
#define PROGMEM 
#undef ARDUINO

/* and we use the buffer sizes for real computers */
#define BUFSIZE         256
#define STACKSIZE       256
#define GOSUBDEPTH      64
#define FORDEPTH        64
#define LINECACHESIZE   64

/* the buffer size for simulated serial RAM */
#define SPIRAMSBSIZE 512

/* 
 * This code measures the fast ticker frequency in microseconds 
 * Activate this only for test purposes. Not really useful on POSIX.
 */
#define FASTTICKERPROFILE

/*
 * Does the platform has command line args and do we want to use them 
 */
#define HASARGS

/*
 * How restrictive are we on function recursive calls to protect the stack
 * On Posix systems we can be more generous.
 */
#define FNLIMIT 128

