//////////////////////////////////////////////////
//                                              //
// Emu64                                        //
// von Thorsten Kattanek                        //
//                                              //
// #file: mmu_class.h                           //
//                                              //
// Dieser Sourcecode ist Copyright geschützt!   //
// Geistiges Eigentum von Th.Kattanek           //
//                                              //
// Letzte Änderung am 23.03.2022                //
// www.emu64.de                                 //
//                                              //
//////////////////////////////////////////////////

#ifndef MMU_CLASS_H
#define MMU_CLASS_H

///#include <functional>
#include "structs.h"
#include "mos6510_class.h"

template<typename T> class ReadProcFn {
public:
    typedef unsigned char (T::*fnProc)(unsigned short);
private:
    fnProc fn;
    T* h;
public:
    ReadProcFn(fnProc fn, T* h) : fn(fn), h(h) {}
    ReadProcFn(const ReadProcFn& c) : fn(c.fn), h(c.h) {}
    ReadProcFn& operator=(const ReadProcFn& c) {
        fn = c.fn;
        h = c.h;
        return *this;
    }
    unsigned char operator()(unsigned short a) { return (h->*fn)(a); }
};

template<typename T> class WriteProcFn {
public:
    typedef void (T::*fnProc)(unsigned short,unsigned char);
private:
    fnProc fn;
    T* h;
public:
    WriteProcFn(fnProc fn, T* h) : fn(fn), h(h) {}
    WriteProcFn(const WriteProcFn& c) : fn(c.fn), h(c.h) {}
    WriteProcFn& operator=(const WriteProcFn& c) {
        fn = c.fn;
        h = c.h;
        return *this;
    }
    void operator()(unsigned short a, unsigned char x) { (h->*fn)(a, x); }
    template<typename T2> WriteProcFn<T2> cast() const {
        WriteProcFn tmp = WriteProcFn(fn, h);
        WriteProcFn<T2>* tmp2 = (WriteProcFn<T2>*)&tmp;
        return *tmp2;
    }
};

template<typename T> class RefreshProcFn {
public:
    typedef void (T::*fnProc)(uint8_t*);
private:
    fnProc fn;
    T* h;
public:
    RefreshProcFn(fnProc fn, T* h) : fn(fn), h(h) {}
    RefreshProcFn(const RefreshProcFn& c) : fn(c.fn), h(c.h) {}
    RefreshProcFn& operator=(const RefreshProcFn& c) {
        fn = c.fn;
        h = c.h;
        return *this;
    }
    void operator()(uint8_t* b) { (h->*fn)(b); }
};

template<typename T> class VVProcFn {
public:
    typedef void (T::*fnProc)(void);
private:
    fnProc fn;
    T* h;
public:
    VVProcFn(fnProc fn, T* h) : fn(fn), h(h) {}
    VVProcFn(const VVProcFn& c) : fn(c.fn), h(c.h) {}
    VVProcFn& operator=(const VVProcFn& c) {
        fn = c.fn;
        h = c.h;
        return *this;
    }
    void operator()(void) { (h->*fn)(); }
};

class VICII;
class C64Class;
class MOS6526;
class CartridgeClass;

class MMU
{
public:
    /// Funktionen ///
    MMU(void);
    ~MMU(void);
    void Reset();

    void ChangeMemMap(void);
    unsigned char* GetFarbramPointer(void);
    unsigned char* GetRAMPointer(void);
    bool LoadKernalRom(const char *filename);
    bool LoadBasicRom(const char *filename);
    bool LoadCharRom(const char *filename);
    unsigned char GetReadSource(unsigned char page);
    unsigned char GetWriteDestination(unsigned char page);
    //bool SaveFreez(FILE* File);
    //bool LoadFreez(FILE *File,unsigned short Version);
    /// Variablen ///

    MOS6510_PORT *CPU_PORT;

    // Zeiger für Read / Write Funktionen ///
    // Diese werden Teilweise Intern und Extern gesetzt ///
    // Ein Aufruf erfolg immer ohne Überprüfung auf gültigen Zeiger !!! //
    ReadProcFn<C64Class> CPUReadProcTbl[0x100];
    WriteProcFn<C64Class> CPUWriteProcTbl[0x100];
    ReadProcFn<C64Class> VICReadProcTbl[0x100];
    WriteProcFn<MMU> VicIOWriteProc;
    ReadProcFn<VICII> VicIOReadProc;
    WriteProcFn<C64Class> SidIOWriteProc;
    ReadProcFn<C64Class> SidIOReadProc;
    WriteProcFn<MOS6526> Cia1IOWriteProc;
    WriteProcFn<MOS6526> Cia2IOWriteProc;
    ReadProcFn<MOS6526> Cia1IOReadProc;
    ReadProcFn<MOS6526> Cia2IOReadProc;
    WriteProcFn<CartridgeClass> CRTRom1WriteProc;
    WriteProcFn<CartridgeClass> CRTRom2WriteProc;
    WriteProcFn<CartridgeClass> CRTRom3WriteProc;
    ReadProcFn<CartridgeClass> CRTRom1ReadProc;
    ReadProcFn<CartridgeClass> CRTRom2ReadProc;
    ReadProcFn<CartridgeClass> CRTRom3ReadProc;
    ReadProcFn<C64Class> IO1ReadProc;
    ReadProcFn<C64Class> IO2ReadProc;
    WriteProcFn<C64Class> IO1WriteProc;
    WriteProcFn<C64Class> IO2WriteProc;
/**
    std::function<unsigned char(unsigned short)>* GetCPUReadProcTable;
    std::function<unsigned char(unsigned short)>* GetVICReadProcTable;
    std::function<void(unsigned short, unsigned char)>* GetCPUWriteProcTable();
*/
    bool *GAME;
    bool *EXROM;
    bool *RAM_H;
    bool *RAM_L;

    unsigned char MEMORY_MAP;
    unsigned char MEMORY_MAP_OLD;

    bool	*EasyFlashDirty1;
    unsigned char *EasyFlashByte1;
    bool	*EasyFlashDirty2;
    unsigned char *EasyFlashByte2;

private:
    /// Funktionen ///
    void InitProcTables(void);
    unsigned char ReadZeroPage(unsigned short adresse);
    void WriteZeroPage(unsigned short adresse, unsigned char wert);
    unsigned char ReadRam(unsigned short adresse);
    void WriteRam(unsigned short adresse, unsigned char wert);
    unsigned char ReadFarbRam(unsigned short adresse);
    void WriteFarbRam(unsigned short adresse, unsigned char wert);
    unsigned char ReadKernalRom(unsigned short adresse);
    unsigned char ReadBasicRom(unsigned short adresse);
    unsigned char ReadCharRom(unsigned short adresse);
    unsigned char ReadCRT1(unsigned short adresse);
    void WriteCRT1(unsigned short adresse, unsigned char wert);
    unsigned char ReadCRT2(unsigned short adresse);
    void WriteCRT2(unsigned short adresse, unsigned char wert);
    unsigned char ReadCRT3(unsigned short adresse);
    void WriteCRT3(unsigned short adresse, unsigned char wert);
    unsigned char ReadVicCharRomBank0(unsigned short adresse);
    unsigned char ReadVicCharRomBank2(unsigned short adresse);
    unsigned char ReadVicRam(unsigned short adresse);
    unsigned char ReadOpenAdress(unsigned short adresse);
    void WriteOpenAdress(unsigned short adresse, unsigned char wert);

	void InitRam(uint8_t init_value, uint16_t invert_value_every, uint16_t random_pattern_legth, uint16_t repeat_random_pattern, uint16_t random_chance);

    /// Variablen ///
    unsigned char RAM[0x10000];			// 64KB
    unsigned char BASIC_ROM[0x2000];		// 8KB ab 0xA000
    unsigned char CHAR_ROM[0x1000];		// 4KB ab 0xD000
    unsigned char FARB_RAM[0x0400];		// 1KB ab 0xD800
    unsigned char KERNAL_ROM[0x2000];		// 8KB ab 0xE000

    /// AktMemoryMap Visuall ///
    unsigned char MapReadSource[0x100];
    unsigned char MapWriteDestination[0x100];
};
#endif // MMU_CLASS_H
