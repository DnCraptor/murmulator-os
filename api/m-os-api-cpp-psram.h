#pragma once
#ifndef m_os_api_psram
#define m_os_api_psram
#include "m-os-api.h"

class uint8_i {
    size_t base;
public:
    inline uint8_i(void) : base(0) {}
    inline uint8_i(size_t b) : base(b) {}
    inline void write(size_t a, uint8_t v) const { write8psram(base + a, v); }
    inline void writeNshift(uint8_t v) { write8psram(base, v); ++base; }
    inline void write(size_t a, uint32_t v) const { write32psram(base + a, v); }
    inline void writeNshift(uint32_t v) { write32psram(base, v); base += 4; }
    template<typename IA> inline uint8_i operator+(IA off) { return uint8_i(base + off); }
    inline uint8_i operator[](size_t a) { return uint8_i(base + a); }
    inline uint8_t get() { return read8psram(base); }
    inline uint32_t get32(size_t a) const { return read32psram(base + a); }
    inline uint8_i& operator&() { return *this; } // no use-cases to get address of (pointer to) this object
    inline operator uint8_t() { return get(); }
    inline void operator=(uint8_t v) { write32psram(base, v); }
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
    inline uint8_i operator[](size_t a) { return uint8_i(base + a); }
    inline operator uint8_i() { return uint8_i(base); }
    template<typename IA> inline uint8_i operator+(IA off) { return uint8_i(base + off); }
};

class psram_a2 {
    size_t sz1;
    size_t sz2;
    size_t base;
public:
    inline psram_a2(size_t sz1, size_t sz2) : sz1(sz1), sz2(sz2), base( psram_manager::get_base(sz1 * sz2) ) { }
    inline ~psram_a2(void) { psram_manager::release_base(base); }
    inline uint8_i operator[](size_t a) { return uint8_i(base + a * sz2); }
};

static size_t fread(const uint8_i psram, size_t sz1, size_t sz2, FIL* file) {
    size_t res = 0;
    sz2 *= sz1;
    uint32_t* buf = new uint32_t[128]();
    while (res < sz2 && FR_OK == f_read(file, buf, 128 << 2, &sz1)) {
        for (size_t i = 0; i < sz1; i += 4) {
            psram.write(res + i, buf[i >> 2]);
        }
        res += sz1;
    }
    delete[] buf;
    return res;
}
static size_t fwrite(const uint8_i psram, size_t sz1, size_t sz2, FIL* file) {
    size_t res = 0;
    sz2 *= sz1;
    uint32_t* buf = new uint32_t[128]();
    for (size_t i = 0; i < 512 && res + i < sz2; i += 4) {
        buf[i >> 2] = psram.get32(res + i);
    }
    while (res < sz2 && FR_OK == f_write(file, buf, sz2 > res + 512 ? 512 : sz2 - res, &sz1)) {
        res += sz1;
        if (res == sz2) break;
        for (size_t i = 0; i < 512 && res + i < sz2; i += 4) {
            buf[i >> 2] = psram.get32(res + i);
        }
    } 
    delete[] buf;
    return res;
}

#endif
