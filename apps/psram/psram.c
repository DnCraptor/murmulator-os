#include "m-os-api.h"
#include <hardware/timer.h>

inline static uint32_t unsigned_divide(
    uint32_t dividend,
    uint32_t divisor,
	uint32_t* pquotient,
    uint32_t* premainder
);

int main() {
    FIL * f = get_stdout();
    uint32_t sz = psram_size();
    fgoutf(f, "PSRAM size: %d bytes\n", sz);
    uint32_t a = 0;
    uint32_t begin = time_us_32();
    for (; a < sz; ++a) {
        write8psram(a, a & 0xFF);
    }
    uint32_t elapsed = time_us_32() - begin;
    uint32_t q, r;
    unsigned_divide(unsigned_divide(100 * a, elapsed, q, r), 100, q, r);
    fgoutf(f, "8-bit line write speed: %d.%02d MBps\n", q, r);

    begin = time_us_32();
    for (a = 0; a < sz; ++a) {
        if ((a & 0xFF) != read8psram(a)) {
            fgoutf(f, "8-bit read failed at %ph\n", a);
            break;
        }
    }
    elapsed = time_us_32() - begin;
    unsigned_divide(unsigned_divide(100 * a, elapsed, q, r), 100, q, r);
    fgoutf(f, "8-bit line read speed: %d.%02d MBps\n", q, r);

    begin = time_us_32();
    for (a = 0; a < sz; a += 2) {
        write16psram(a, a & 0xFFFF);
    }
    elapsed = time_us_32() - begin;
    unsigned_divide(unsigned_divide(100 * a, elapsed, q, r), 100, q, r);
    fgoutf(f, "16-bit line write speed: %d.%02d MBps\n", q, r);

    begin = time_us_32();
    for (a = 0; a < sz; a += 2) {
        if ((a & 0xFFFF) != read16psram(a)) {
            fgoutf(f, "16-bit read failed at %ph\n", a);
            break;
        }
    }
    elapsed = time_us_32() - begin;
    unsigned_divide(unsigned_divide(100 * a, elapsed, q, r), 100, q, r);
    fgoutf(f, "16-bit line read speed: %d.%02d MBps\n", q, r);

    begin = time_us_32();
    for (a = 0; a < sz; a += 4) {
        write32psram(a, a);
    }
    elapsed = time_us_32() - begin;
    unsigned_divide(unsigned_divide(100 * a, elapsed, q, r), 100, q, r);
    fgoutf(f, "32-bit line write speed: %d.%02d MBps\n", q, r);

    begin = time_us_32();
    for (a = 0; a < sz; a += 4) {
        if (a != read32psram(a)) {
            fgoutf(f, "32-bit read failed at %ph\n", a);
            break;
        }
    }
    elapsed = time_us_32() - begin;
    unsigned_divide(unsigned_divide(100 * a, elapsed, q, r), 100, q, r);
    fgoutf(f, "32-bit line read speed: %d.%02d MBps\n", q, r);
}

/*
   Copyright stuff

   Use of this program, for any purpose, is granted the author,
   Ian Kaplan, as long as this copyright notice is included in
   the source code or any source code derived from this program.
   The user assumes all responsibility for using this code.

   Ian Kaplan, October 1996

*/
inline static uint32_t unsigned_divide(
    uint32_t dividend,
    uint32_t divisor,
	uint32_t* pquotient,
    uint32_t* premainder
) {
  unsigned int t, num_bits;
  unsigned int q, bit, d;
  int i;

  *premainder = 0;
  *pquotient = 0;

  if (divisor == 0)
    *pquotient = 0xFFFFFFFF; // W/A
    return *pquotient;

  if (divisor > dividend) {
    *premainder = dividend;
    return *pquotient;
  }

  if (divisor == dividend) {
    *pquotient = 1;
    return *pquotient;
  }

  num_bits = 32;

  while (*premainder < divisor) {
    bit = (dividend & 0x80000000) >> 31;
    *premainder = (*premainder << 1) | bit;
    d = dividend;
    dividend = dividend << 1;
    num_bits--;
  }


  /* The loop, above, always goes one iteration too far.
     To avoid inserting an "if" statement inside the loop
     the last iteration is simply reversed. */

  dividend = d;
  *premainder = *premainder >> 1;
  num_bits++;

  for (i = 0; i < num_bits; i++) {
    bit = (dividend & 0x80000000) >> 31;
    *premainder = (*premainder << 1) | bit;
    t = *premainder - divisor;
    q = !((t & 0x80000000) >> 31);
    dividend = dividend << 1;
    *pquotient = (*pquotient << 1) | q;
    if (q) {
       *premainder = t;
     }
  }
  return *pquotient;
}  /* unsigned_divide */
