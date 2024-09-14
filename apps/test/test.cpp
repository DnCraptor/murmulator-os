#include "m-os-api.h"

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
    return 0;
}
/** Program output (as expected):
CPU::readRegister[3]->1
CPU::readRegister[4]->1
FDD::readReg[8]->6
FDD::readIO[9]->7
FDD::readIO[10]->7
*/
