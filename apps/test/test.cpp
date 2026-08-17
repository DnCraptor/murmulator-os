#include "m-os-api.h"
#include "m-os-api-math.h"

template<typename T> class ReadProcFn {
public:
    typedef unsigned char (T::*fnProc)(unsigned short);
private:
    fnProc fn;
    T* h;
public:
    ReadProcFn(void) : fn(NULL), h(NULL) {}
    ReadProcFn(fnProc fn, T* h) : fn(fn), h(h) {}
    ReadProcFn(const ReadProcFn& c) : fn(c.fn), h(c.h) {}
    ReadProcFn& operator=(const ReadProcFn& c) {
        fn = c.fn;
        h = c.h;
        return *this;
    }
    unsigned char operator()(unsigned short a) { return (h->*fn)(a); }
    template<typename T2> ReadProcFn operator=(const ReadProcFn<T2>& c) {
        *this = *(ReadProcFn<T>*)&c;
        return *this;
    }
};

class CPU {
    unsigned char reg;
    unsigned char ram;
public:
    CPU(): reg(1), ram(2) {}
    unsigned char readRegister(unsigned short a) {
        printf("CPU::readRegister[%d]->%d\n", a, reg);
        return reg;
    }
    unsigned char readRAM(unsigned short a) {
        printf("CPU::readRAM[%d]->%d\n", a, ram);
        return ram;
    }
};

class FDD {
    unsigned char reg;
    unsigned char io;
public:
    FDD(): reg(6), io(7) {}
    unsigned char readReg(unsigned short a) {
        printf("FDD::readReg[%d]->%d\n", a, reg);
        return reg;
    }
    unsigned char readIO(unsigned short a) {
        printf("FDD::readIO[%d]->%d\n", a, io);
        return io;
    }
};

int main(void) {
    // notmal flow, type-safe
    CPU cpu;
    ReadProcFn<CPU> rReg(&CPU::readRegister, &cpu);
    ReadProcFn<CPU> rRam(&CPU::readRAM, &cpu);
    FDD fdd;
    ReadProcFn<FDD> rFReg(&FDD::readReg, &fdd);
    ReadProcFn<FDD> rFIO(&FDD::readIO, &fdd);
    rReg(3);
    rReg(4);
    rFReg(8);
    rFIO(9);
    // let reassign: (not type-safe)
    rReg = rFIO; // now rReg should call FDD::readIO
    rReg(10);

    volatile int s32n = -100000003;
    volatile int s32d = 97;
    int s32q = s32n / s32d;
    int s32r = s32n % s32d;
    if (s32q != -1030927 || s32r != -84) {
        printf("signed 32-bit divmod: FAIL (%d, %d)\n", s32q, s32r);
        return 1;
    }

    volatile unsigned u32n = 0xF1234567u;
    volatile unsigned u32d = 1009u;
    unsigned u32q = u32n / u32d;
    unsigned u32r = u32n % u32d;
    if (u32q != 4009534u || u32r != 777u) {
        printf("unsigned 32-bit divmod: FAIL (%u, %u)\n", u32q, u32r);
        return 1;
    }

    volatile long long s64n = -0x123456789ABCDEFLL;
    volatile long long s64d = 1000003LL;
    long long s64q = s64n / s64d;
    long long s64r = s64n % s64d;
    if (s64q != -81985283260LL || s64r != -637115LL) {
        printf("signed 64-bit divmod: FAIL\n");
        return 1;
    }

    volatile unsigned long long u64n = 0xFEDCBA9876543210ULL;
    volatile unsigned long long u64d = 10000019ULL;
    unsigned long long u64q = u64n / u64d;
    unsigned long long u64r = u64n % u64d;
    if (u64q != 1836472365151ULL || u64r != 8126851ULL) {
        printf("unsigned 64-bit divmod: FAIL\n");
        return 1;
    }

    printf("System API v%d divmod: OK\n", M_API_VERSION);
    return 0;
}
/** Program output (as expected):
CPU::readRegister[3]->1
CPU::readRegister[4]->1
FDD::readReg[8]->6
FDD::readIO[9]->7
FDD::readIO[10]->7
*/
