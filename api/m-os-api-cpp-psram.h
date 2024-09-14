#pragma once
#ifndef m_os_api_psram
#define m_os_api_psram
#include "m-os-api.h"

class uint8_i {
    size_t base;
public:
    inline uint8_i(size_t b) : base(b) {}
};

class psram_manager {
    static size_t off;
public:
    inline static size_t get_base(size_t sz) {
        size_t res = off;
        off += sz;
        return res;
    }
    inline static void release_base(size_t base) {
        // TODO: release;
    }
};
size_t psram_manager::off = 0;

class psram {
    size_t base;
public:
    inline psram(size_t sz) : base( psram_manager::get_base(sz) ) { }
    inline ~psram(void) { psram_manager::release_base(base); }
    inline uint8_t operator[](size_t a) { return read8psram(base + a); }
  ///  inline uint8_i& operator[](size_t a) { return uint8_i(base + a); }
};

#endif
