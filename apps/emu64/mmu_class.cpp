//////////////////////////////////////////////////
//                                              //
// Emu64                                        //
// von Thorsten Kattanek                        //
//                                              //
// #file: mmu_class.cpp                         //
//                                              //
// Dieser Sourcecode ist Copyright geschützt!   //
// Geistiges Eigentum von Th.Kattanek           //
//                                              //
// www.emu64.de                                 //
//                                              //
//////////////////////////////////////////////////

#include "mmu_class.h"
#include "m-os-api-timer.h"
///#include <fstream>

class VICII;

MMU::MMU(void) :
    VicIOWriteProc ( WriteProcFn<MMU>(&MMU::WriteRam, this) ),
    SidIOWriteProc ( WriteProcFn<MMU>(&MMU::WriteRam, this) ),
    Cia1IOWriteProc ( WriteProcFn<MMU>(&MMU::WriteRam, this) ),
    Cia2IOWriteProc ( WriteProcFn<MMU>(&MMU::WriteRam, this) ),
    VicIOReadProc ( ReadProcFn<MMU>(&MMU::ReadRam, this) ),
    SidIOReadProc ( ReadProcFn<MMU>(&MMU::ReadRam, this) ),
    Cia1IOReadProc ( ReadProcFn<MMU>(&MMU::ReadRam, this) ),
    Cia2IOReadProc ( ReadProcFn<MMU>(&MMU::ReadRam, this) )
{
    for(int i = 0; i < 0x10000; ++i) RAM[i] = 0;

    MEMORY_MAP = MEMORY_MAP_OLD = 0;

    InitProcTables();

    srand (time(NULL));
}

MMU::~MMU(void)
{
}

void MMU::Reset()
{
	CPU_PORT->Reset();
	ChangeMemMap();
	InitRam(0xff, 64, 1, 256, 0); // Funktioniert mit allen VICE RamPattern Tests
}

unsigned char* MMU::GetFarbramPointer(void)
{
	return FARB_RAM;
}

static bool load_file (const char* filename, uint8_t* to, size_t exp_sz) {
        FILE *file;
        if (FR_OK != f_open (file, filename, FA_READ)) return false;
        UINT reading_bytes;
        f_read (file, to, exp_sz, &reading_bytes);
	if(exp_sz != reading_bytes) {
        	f_close(file);
		return false;
	}
	f_close(file);
	return true;
}

bool MMU::LoadKernalRom(const char* filename)
{
        return load_file(filename, KERNAL_ROM, 0x2000);
}

bool MMU::LoadBasicRom(const char* filename)
{
        return load_file(filename, BASIC_ROM, 0x2000);
}

bool MMU::LoadCharRom(const char* filename)
{
        return load_file(filename, CHAR_ROM, 0x1000);
}

/*
bool MMU::SaveFreez(FILE* File)
{
	fwrite(RAM,1,0x10000,File);
	fwrite(FARB_RAM,1,0x0400,File);
	return true;
}

bool MMU::LoadFreez(FILE* File,unsigned short Version)
{
	switch(Version)
	{
	case 0x0100:
	case 0x0101:
		ChangeMemMap();
		fread(RAM,1,0x10000,File);
		fread(FARB_RAM,1,0x0400,File);
		break;
	}
	return true;
}
*/

unsigned char* MMU::GetRAMPointer(void)
{
    return RAM;
}

unsigned char MMU::GetReadSource(unsigned char page)
{
    return MapReadSource[page];
}

unsigned char MMU::GetWriteDestination(unsigned char page)
{
    return MapWriteDestination[page];
}

///////////////////// Intern ////////////////////////

void MMU::ChangeMemMap()
{
        CPU_PORT->ConfigChanged(CPU_PORT->TAPE_SENSE, 1, 0x17);
        unsigned char AE = ((~CPU_PORT->DIR | CPU_PORT->DATA) & 0x7);

        MEMORY_MAP = 0;
        if (*EXROM) MEMORY_MAP|=2;
        if (*GAME)  MEMORY_MAP|=1;
        if ((AE&4)==4) MEMORY_MAP|=4;
        if ((AE&2)==2) MEMORY_MAP|=8;
        if ((AE&1)==1) MEMORY_MAP|=16;

        if(MEMORY_MAP == MEMORY_MAP_OLD) return;
	
	if ( MEMORY_MAP_OLD == 2 || MEMORY_MAP_OLD == 18 || MEMORY_MAP_OLD == 10 || MEMORY_MAP_OLD == 6 || MEMORY_MAP_OLD == 26 || MEMORY_MAP_OLD == 22 || MEMORY_MAP_OLD == 14 || MEMORY_MAP_OLD == 30 )
	{
		for(int i=0;i<112;i++)
		{
                        CPUReadProcTbl[0x10+i] = ReadProcFn<MMU>(&MMU::ReadRam, this);
                        CPUWriteProcTbl[0x10+i] = WriteProcFn<MMU>(&MMU::WriteRam, this);
                        MapReadSource[0x10+i] = MV_RAM;
                        MapWriteDestination[0x10+i] = MV_RAM;
		}
		for(int i=0;i<16;i++)
		{
                        CPUReadProcTbl[0xC0+i] = ReadProcFn<MMU>(&MMU::ReadRam, this);
                        CPUWriteProcTbl[0xC0+i] = WriteProcFn<MMU>(&MMU::WriteRam, this);
                        MapReadSource[0xC0+i] = MV_RAM;
                        MapWriteDestination[0xC0+i] = MV_RAM;
        	}
	}

        if ( MEMORY_MAP == 31 )	{
			/// READ
			for(int i=0;i<32;++i)
			{
                                CPUReadProcTbl[0x80+i] = ReadProcFn<MMU>(&MMU::ReadRam,this);
                                CPUReadProcTbl[0xA0+i] = ReadProcFn<MMU>(&MMU::ReadBasicRom,this);
                                CPUReadProcTbl[0xE0+i] = ReadProcFn<MMU>(&MMU::ReadKernalRom,this);
                                MapReadSource[0x80+i] = MV_RAM;
                                MapReadSource[0xA0+i] = MV_BASIC_ROM;
                                MapReadSource[0xE0+i] = MV_KERNAL_ROM;
			}
			for(int i=0;i<4;++i)
			{
                                CPUReadProcTbl[0xD0+i] = VicIOReadProc;
                                CPUReadProcTbl[0xD4+i] = SidIOReadProc;
                                CPUReadProcTbl[0xD8+i] = ReadProcFn<MMU>(&MMU::ReadFarbRam,this);
                                MapReadSource[0xD0+i] = MV_VIC;
                                MapReadSource[0xD4+i] = MV_SID;
                                MapReadSource[0xD8+i] = MV_FARB_RAM;
			}
                        CPUReadProcTbl[0xDC] = Cia1IOReadProc;
                        CPUReadProcTbl[0xDD] = Cia2IOReadProc;
                        CPUReadProcTbl[0xDE] = IO1ReadProc;
                        CPUReadProcTbl[0xDF] = IO2ReadProc;
                        MapReadSource[0xDC] = MV_CIA1;
                        MapReadSource[0xDD] = MV_CIA2;
                        MapReadSource[0xDE] = MV_IO1;
                        MapReadSource[0xDF] = MV_IO2;

			/// WRITE
			for(int i=0;i<32;++i)
			{
                                CPUWriteProcTbl[0x80+i] = WriteProcFn<MMU>(&MMU::WriteRam,this);
                                CPUWriteProcTbl[0xA0+i] = WriteProcFn<MMU>(&MMU::WriteRam,this);
                                CPUWriteProcTbl[0xE0+i] = WriteProcFn<MMU>(&MMU::WriteRam,this);
                                MapWriteDestination[0x80+i] = MV_RAM;
                                MapWriteDestination[0xA0+i] = MV_RAM;
                                MapWriteDestination[0xE0+i] = MV_RAM;
			}
			for(int i=0;i<4;++i)
			{
                                CPUWriteProcTbl[0xD0+i] = VicIOWriteProc;
                                CPUWriteProcTbl[0xD4+i] = SidIOWriteProc;
                                CPUWriteProcTbl[0xD8+i] = WriteProcFn<MMU>(&MMU::WriteFarbRam,this);
                                MapWriteDestination[0xD0+i] = MV_VIC;
                                MapWriteDestination[0xD4+i] = MV_SID;
                                MapWriteDestination[0xD8+i] = MV_FARB_RAM;
			}
                        CPUWriteProcTbl[0xDC] = Cia1IOWriteProc;
                        CPUWriteProcTbl[0xDD] = Cia2IOWriteProc;
                        CPUWriteProcTbl[0xDE] = IO1WriteProc;
                        CPUWriteProcTbl[0xDF] = IO2WriteProc;
                        MapWriteDestination[0xDC] = MV_CIA1;
                        MapWriteDestination[0xDD] = MV_CIA2;
                        MapWriteDestination[0xDE] = MV_IO1;
                        MapWriteDestination[0xDF] = MV_IO2;
	}
        else if ( MEMORY_MAP == 27 ) {
			/// READ
			for(int i=0;i<32;++i)
			{
                                CPUReadProcTbl[0x80+i] = ReadProcFn<MMU>(&MMU::ReadRam,this);
                                CPUReadProcTbl[0xA0+i] = ReadProcFn<MMU>(&MMU::ReadBasicRom,this);
                                CPUReadProcTbl[0xE0+i] = ReadProcFn<MMU>(&MMU::ReadKernalRom,this);
                                MapReadSource[0x80+i] = MV_RAM;
                                MapReadSource[0xA0+i] = MV_BASIC_ROM;
                                MapReadSource[0xE0+i] = MV_KERNAL_ROM;
			}
			for(int i=0;i<16;++i)
			{
                                CPUReadProcTbl[0xD0+i] = ReadProcFn<MMU>(&MMU::ReadCharRom,this);
                                MapReadSource[0xD0+i] = MV_CHAR_ROM;
			}

			/// WRITE
			for(int i=0;i<32;++i)
			{
                                CPUWriteProcTbl[0x80+i] = WriteProcFn<MMU>(&MMU::WriteRam,this);
                                CPUWriteProcTbl[0xA0+i] = WriteProcFn<MMU>(&MMU::WriteRam,this);
                                CPUWriteProcTbl[0xE0+i] = WriteProcFn<MMU>(&MMU::WriteRam,this);
                                MapWriteDestination[0x80+i] = MV_RAM;
                                MapWriteDestination[0xA0+i] = MV_RAM;
                                MapWriteDestination[0xE0+i] = MV_RAM;
			}
			for(int i=0;i<16;++i)
			{
                                CPUWriteProcTbl[0xD0+i] = WriteProcFn<MMU>(&MMU::WriteRam,this);
                                MapWriteDestination[0xD0+i] = MV_RAM;
			}
        }
        else if ( MEMORY_MAP == 29 ) {
			/// READ
			for(int i=0;i<32;++i)
			{
                                CPUReadProcTbl[0x80+i] = ReadProcFn<MMU>(&MMU::ReadCRT1,this);
                                CPUReadProcTbl[0xA0+i] = ReadProcFn<MMU>(&MMU::ReadBasicRom,this);
                                CPUReadProcTbl[0xE0+i] = ReadProcFn<MMU>(&MMU::ReadKernalRom,this);
                                MapReadSource[0x80+i] = MV_CRT_1;
                                MapReadSource[0xA0+i] = MV_BASIC_ROM;
                                MapReadSource[0xE0+i] = MV_KERNAL_ROM;
			}
			for(int i=0;i<4;++i)
			{
                                CPUReadProcTbl[0xD0+i] = VicIOReadProc;
                                CPUReadProcTbl[0xD4+i] = SidIOReadProc;
                                CPUReadProcTbl[0xD8+i] = ReadProcFn<MMU>(&MMU::ReadFarbRam,this);
                                MapReadSource[0xD0+i] = MV_VIC;
                                MapReadSource[0xD4+i] = MV_SID;
                                MapReadSource[0xD8+i] = MV_FARB_RAM;
			}
                        CPUReadProcTbl[0xDC] = Cia1IOReadProc;
                        CPUReadProcTbl[0xDD] = Cia2IOReadProc;
                        CPUReadProcTbl[0xDE] = IO1ReadProc;
                        CPUReadProcTbl[0xDF] = IO2ReadProc;
                        MapReadSource[0xDC] = MV_CIA1;
                        MapReadSource[0xDD] = MV_CIA2;
                        MapReadSource[0xDE] = MV_IO1;
                        MapReadSource[0xDF] = MV_IO2;

			/// WRITE
			for(int i=0;i<32;++i)
			{
                                CPUWriteProcTbl[0x80+i] = WriteProcFn<MMU>(&MMU::WriteCRT1,this);
                                CPUWriteProcTbl[0xA0+i] = WriteProcFn<MMU>(&MMU::WriteRam,this);
                                CPUWriteProcTbl[0xE0+i] = WriteProcFn<MMU>(&MMU::WriteRam,this);
                                MapWriteDestination[0x80+i] = MV_CRT_1;
                                MapWriteDestination[0xA0+i] = MV_RAM;
                                MapWriteDestination[0xE0+i] = MV_RAM;
			}
			for(int i=0;i<4;++i)
			{
                                CPUWriteProcTbl[0xD0+i] = VicIOWriteProc;
                                CPUWriteProcTbl[0xD4+i] = SidIOWriteProc;
                                CPUWriteProcTbl[0xD8+i] = WriteProcFn<MMU>(&MMU::WriteFarbRam,this);
                                MapWriteDestination[0xD0+i] = MV_VIC;
                                MapWriteDestination[0xD4+i] = MV_SID;
                                MapWriteDestination[0xD8+i] = MV_FARB_RAM;
			}
                        CPUWriteProcTbl[0xDC] = Cia1IOWriteProc;
                        CPUWriteProcTbl[0xDD] = Cia2IOWriteProc;
                        CPUWriteProcTbl[0xDE] = IO1WriteProc;
                        CPUWriteProcTbl[0xDF] = IO2WriteProc;
                        MapWriteDestination[0xDC] = MV_CIA1;
                        MapWriteDestination[0xDD] = MV_CIA2;
                        MapWriteDestination[0xDE] = MV_IO1;
                        MapWriteDestination[0xDF] = MV_IO2;
        }
        else if ( MEMORY_MAP == 25 ) {
			/// READ
			for(int i=0;i<32;++i)
			{
                                CPUReadProcTbl[0x80+i] = ReadProcFn<MMU>(&MMU::ReadCRT1,this);
                                CPUReadProcTbl[0xA0+i] = ReadProcFn<MMU>(&MMU::ReadBasicRom,this);
                                CPUReadProcTbl[0xE0+i] = ReadProcFn<MMU>(&MMU::ReadKernalRom,this);
                                MapReadSource[0x80+i] = MV_CRT_1;
                                MapReadSource[0xA0+i] = MV_BASIC_ROM;
                                MapReadSource[0xE0+i] = MV_KERNAL_ROM;
			}
			for(int i=0;i<16;++i)
			{
                                CPUReadProcTbl[0xD0+i] = ReadProcFn<MMU>(&MMU::ReadCharRom,this);
                                MapReadSource[0xD0+i] = MV_CHAR_ROM;
			}

			/// WRITE
			for(int i=0;i<32;++i)
			{
                                CPUWriteProcTbl[0x80+i] = WriteProcFn<MMU>(&MMU::WriteCRT1,this);
                                CPUWriteProcTbl[0xA0+i] = WriteProcFn<MMU>(&MMU::WriteRam,this);
                                CPUWriteProcTbl[0xE0+i] = WriteProcFn<MMU>(&MMU::WriteRam,this);
                                MapWriteDestination[0x80+i] = MV_CRT_1;
                                MapWriteDestination[0xA0+i] = MV_RAM;
                                MapWriteDestination[0xE0+i] = MV_RAM;
			}
			for(int i=0;i<16;++i)
			{
                                CPUWriteProcTbl[0xD0+i] = WriteProcFn<MMU>(&MMU::WriteRam,this);
                                MapWriteDestination[0xD0+i] = MV_RAM;
			}
        }
        else if ( MEMORY_MAP == 28 ) {
			/// READ
			for(int i=0;i<32;++i)
			{
                                CPUReadProcTbl[0x80+i] = ReadProcFn<MMU>(&MMU::ReadCRT1,this);
                                CPUReadProcTbl[0xA0+i] = ReadProcFn<MMU>(&MMU::ReadCRT2,this);
                                CPUReadProcTbl[0xE0+i] = ReadProcFn<MMU>(&MMU::ReadKernalRom,this);
                                MapReadSource[0x80+i] = MV_CRT_1;
                                MapReadSource[0xA0+i] = MV_CRT_2;
                                MapReadSource[0xE0+i] = MV_KERNAL_ROM;
			}
			for(int i=0;i<4;++i)
			{
                                CPUReadProcTbl[0xD0+i] = VicIOReadProc;
                                CPUReadProcTbl[0xD4+i] = SidIOReadProc;
                                CPUReadProcTbl[0xD8+i] = ReadProcFn<MMU>(&MMU::ReadFarbRam,this);
                                MapReadSource[0xD0+i] = MV_VIC;
                                MapReadSource[0xD4+i] = MV_SID;
                                MapReadSource[0xD8+i] = MV_FARB_RAM;
			}
                        CPUReadProcTbl[0xDC] = Cia1IOReadProc;
                        CPUReadProcTbl[0xDD] = Cia2IOReadProc;
                        CPUReadProcTbl[0xDE] = IO1ReadProc;
                        CPUReadProcTbl[0xDF] = IO2ReadProc;
                        MapReadSource[0xDC] = MV_CIA1;
                        MapReadSource[0xDD] = MV_CIA2;
                        MapReadSource[0xDE] = MV_IO1;
                        MapReadSource[0xDF] = MV_IO2;

			/// WRITE
			for(int i=0;i<32;++i)
			{
                                CPUWriteProcTbl[0x80+i] = WriteProcFn<MMU>(&MMU::WriteCRT1,this);
                                CPUWriteProcTbl[0xA0+i] = WriteProcFn<MMU>(&MMU::WriteCRT2,this);
                                CPUWriteProcTbl[0xE0+i] = WriteProcFn<MMU>(&MMU::WriteRam,this);
                                MapWriteDestination[0x80+i] = MV_CRT_1;
                                MapWriteDestination[0xA0+i] = MV_CRT_2;
                                MapWriteDestination[0xE0+i] = MV_RAM;
			}
			for(int i=0;i<4;++i)
			{
                                CPUWriteProcTbl[0xD0+i] = VicIOWriteProc;
                                CPUWriteProcTbl[0xD4+i] = SidIOWriteProc;
                                CPUWriteProcTbl[0xD8+i] = WriteProcFn<MMU>(&MMU::WriteFarbRam,this);
                                MapWriteDestination[0xD0+i] = MV_VIC;
                                MapWriteDestination[0xD4+i] = MV_SID;
                                MapWriteDestination[0xD8+i] = MV_FARB_RAM;
			}
                        CPUWriteProcTbl[0xDC] = Cia1IOWriteProc;
                        CPUWriteProcTbl[0xDD] = Cia2IOWriteProc;
                        CPUWriteProcTbl[0xDE] = IO1WriteProc;
                        CPUWriteProcTbl[0xDF] = IO2WriteProc;
                        MapWriteDestination[0xDC] = MV_CIA1;
                        MapWriteDestination[0xDD] = MV_CIA2;
                        MapWriteDestination[0xDE] = MV_IO1;
                        MapWriteDestination[0xDF] = MV_IO2;
        }
        else if ( MEMORY_MAP == 24 ) {
			/// READ
			for(int i=0;i<32;++i)
			{
                                CPUReadProcTbl[0x80+i] = ReadProcFn<MMU>(&MMU::ReadCRT1,this);
                                CPUReadProcTbl[0xA0+i] = ReadProcFn<MMU>(&MMU::ReadCRT2,this);
                                CPUReadProcTbl[0xE0+i] = ReadProcFn<MMU>(&MMU::ReadKernalRom,this);
                                MapReadSource[0x80+i] = MV_CRT_1;
                                MapReadSource[0xA0+i] = MV_CRT_2;
                                MapReadSource[0xE0+i] = MV_KERNAL_ROM;
			}
			for(int i=0;i<16;++i)
			{
                                CPUReadProcTbl[0xD0+i] = ReadProcFn<MMU>(&MMU::ReadCharRom,this);
                                MapReadSource[0xD0+i] = MV_CHAR_ROM;
			}

			/// WRITE
			for(int i=0;i<32;++i)
			{
                                CPUWriteProcTbl[0x80+i] = WriteProcFn<MMU>(&MMU::WriteCRT1,this);
                                CPUWriteProcTbl[0xA0+i] = WriteProcFn<MMU>(&MMU::WriteCRT2,this);
                                CPUWriteProcTbl[0xE0+i] = WriteProcFn<MMU>(&MMU::WriteRam,this);
                                MapWriteDestination[0x80+i] = MV_CRT_1;
                                MapWriteDestination[0xA0+i] = MV_CRT_2;
                                MapWriteDestination[0xE0+i] = MV_RAM;
			}
			for(int i=0;i<16;++i)
			{
                                CPUWriteProcTbl[0xD0+i] = WriteProcFn<MMU>(&MMU::WriteRam,this);
                                MapWriteDestination[0xD0+i] = MV_RAM;
			}
        }
        else if ( MEMORY_MAP == 12 ) {
			/// READ
			for(int i=0;i<32;++i)
			{
                                CPUReadProcTbl[0x80+i] = ReadProcFn<MMU>(&MMU::ReadRam,this);
                                CPUReadProcTbl[0xA0+i] = ReadProcFn<MMU>(&MMU::ReadCRT2,this);
                                CPUReadProcTbl[0xE0+i] = ReadProcFn<MMU>(&MMU::ReadKernalRom,this);
                                MapReadSource[0x80+i] = MV_RAM;
                                MapReadSource[0xA0+i] = MV_CRT_2;
                                MapReadSource[0xE0+i] = MV_KERNAL_ROM;
			}
			for(int i=0;i<4;++i)
			{
                                CPUReadProcTbl[0xD0+i] = VicIOReadProc;
                                CPUReadProcTbl[0xD4+i] = SidIOReadProc;
                                CPUReadProcTbl[0xD8+i] = ReadProcFn<MMU>(&MMU::ReadFarbRam,this);
                                MapReadSource[0xD0+i] = MV_VIC;
                                MapReadSource[0xD4+i] = MV_SID;
                                MapReadSource[0xD8+i] = MV_FARB_RAM;
			}
                        CPUReadProcTbl[0xDC] = Cia1IOReadProc;
                        CPUReadProcTbl[0xDD] = Cia2IOReadProc;
                        CPUReadProcTbl[0xDE] = IO1ReadProc;
                        CPUReadProcTbl[0xDF] = IO2ReadProc;
                        MapReadSource[0xDC] = MV_CIA1;
                        MapReadSource[0xDD] = MV_CIA2;
                        MapReadSource[0xDE] = MV_IO1;
                        MapReadSource[0xDF] = MV_IO2;

			/// WRITE
			for(int i=0;i<32;++i)
			{
                                CPUWriteProcTbl[0x80+i] = WriteProcFn<MMU>(&MMU::WriteRam,this);
                                CPUWriteProcTbl[0xA0+i] = WriteProcFn<MMU>(&MMU::WriteCRT2,this);
                                CPUWriteProcTbl[0xE0+i] = WriteProcFn<MMU>(&MMU::WriteRam,this);
                                MapWriteDestination[0x80+i] = MV_RAM;
                                MapWriteDestination[0xA0+i] = MV_CRT_2;
                                MapWriteDestination[0xE0+i] = MV_RAM;
			}
			for(int i=0;i<4;++i)
			{
                                CPUWriteProcTbl[0xD0+i] = VicIOWriteProc;
                                CPUWriteProcTbl[0xD4+i] = SidIOWriteProc;
                                CPUWriteProcTbl[0xD8+i] = WriteProcFn<MMU>(&MMU::WriteFarbRam,this);
                                MapWriteDestination[0xD0+i] = MV_VIC;
                                MapWriteDestination[0xD4+i] = MV_SID;
                                MapWriteDestination[0xD8+i] = MV_FARB_RAM;
			}
                        CPUWriteProcTbl[0xDC] = Cia1IOWriteProc;
                        CPUWriteProcTbl[0xDD] = Cia2IOWriteProc;
                        CPUWriteProcTbl[0xDE] = IO1WriteProc;
                        CPUWriteProcTbl[0xDF] = IO2WriteProc;
                        MapWriteDestination[0xDC] = MV_CIA1;
                        MapWriteDestination[0xDD] = MV_CIA2;
                        MapWriteDestination[0xDE] = MV_IO1;
                        MapWriteDestination[0xDF] = MV_IO2;
        }
        else if ( MEMORY_MAP == 8 ) {
			/// READ
			for(int i=0;i<32;++i)
			{
                                CPUReadProcTbl[0x80+i] = ReadProcFn<MMU>(&MMU::ReadRam,this);
                                CPUReadProcTbl[0xA0+i] = ReadProcFn<MMU>(&MMU::ReadCRT2,this);
                                CPUReadProcTbl[0xE0+i] = ReadProcFn<MMU>(&MMU::ReadKernalRom,this);
                                MapReadSource[0x80+i] = MV_RAM;
                                MapReadSource[0xA0+i] = MV_CRT_2;
                                MapReadSource[0xE0+i] = MV_KERNAL_ROM;
			}
			for(int i=0;i<16;++i)
			{
                                CPUReadProcTbl[0xD0+i] = ReadProcFn<MMU>(&MMU::ReadCharRom,this);
                                MapReadSource[0xD0+i] = MV_CHAR_ROM;
			}

			/// WRITE

			for(int i=0;i<32;++i)
			{
                                CPUWriteProcTbl[0x80+i] = WriteProcFn<MMU>(&MMU::WriteRam,this);
                                CPUWriteProcTbl[0xA0+i] = WriteProcFn<MMU>(&MMU::WriteCRT2,this);
                                CPUWriteProcTbl[0xE0+i] = WriteProcFn<MMU>(&MMU::WriteRam,this);
                                MapWriteDestination[0x80+i] = MV_RAM;
                                MapWriteDestination[0xA0+i] = MV_CRT_2;
                                MapWriteDestination[0xE0+i] = MV_RAM;
			}
			for(int i=0;i<16;++i)
			{
                                CPUWriteProcTbl[0xD0+i] = WriteProcFn<MMU>(&MMU::WriteRam,this);
                                MapWriteDestination[0xD0+i] = MV_RAM;
			}
        }
        else if ( MEMORY_MAP == 13 || MEMORY_MAP == 15 ) {
			/// READ
			for(int i=0;i<32;++i)
			{
                                CPUReadProcTbl[0x80+i] = ReadProcFn<MMU>(&MMU::ReadRam,this);
                                CPUReadProcTbl[0xA0+i] = ReadProcFn<MMU>(&MMU::ReadRam,this);
                                CPUReadProcTbl[0xE0+i] = ReadProcFn<MMU>(&MMU::ReadKernalRom,this);
                                MapReadSource[0x80+i] = MV_RAM;
                                MapReadSource[0xA0+i] = MV_RAM;
                                MapReadSource[0xE0+i] = MV_KERNAL_ROM;
			}
			for(int i=0;i<4;++i)
			{
                                CPUReadProcTbl[0xD0+i] = VicIOReadProc;
                                CPUReadProcTbl[0xD4+i] = SidIOReadProc;
                                CPUReadProcTbl[0xD8+i] = ReadProcFn<MMU>(&MMU::ReadFarbRam,this);
                                MapReadSource[0xD0+i] = MV_VIC;
                                MapReadSource[0xD4+i] = MV_SID;
                                MapReadSource[0xD8+i] = MV_FARB_RAM;
			}
                        CPUReadProcTbl[0xDC] = Cia1IOReadProc;
                        CPUReadProcTbl[0xDD] = Cia2IOReadProc;
                        CPUReadProcTbl[0xDE] = IO1ReadProc;
                        CPUReadProcTbl[0xDF] = IO2ReadProc;
                        MapReadSource[0xDC] = MV_CIA1;
                        MapReadSource[0xDD] = MV_CIA2;
                        MapReadSource[0xDE] = MV_IO1;
                        MapReadSource[0xDF] = MV_IO2;

			/// WRITE
			for(int i=0;i<32;++i)
			{
                                CPUWriteProcTbl[0x80+i] = WriteProcFn<MMU>(&MMU::WriteRam,this);
                                CPUWriteProcTbl[0xA0+i] = WriteProcFn<MMU>(&MMU::WriteRam,this);
                                CPUWriteProcTbl[0xE0+i] = WriteProcFn<MMU>(&MMU::WriteRam,this);
                                MapWriteDestination[0x80+i] = MV_RAM;
                                MapWriteDestination[0xA0+i] = MV_RAM;
                                MapWriteDestination[0xE0+i] = MV_RAM;
			}
			for(int i=0;i<4;++i)
			{
                                CPUWriteProcTbl[0xD0+i] = VicIOWriteProc;
                                CPUWriteProcTbl[0xD4+i] = SidIOWriteProc;
                                CPUWriteProcTbl[0xD8+i] = WriteProcFn<MMU>(&MMU::WriteFarbRam,this);
                                MapWriteDestination[0xD0+i] = MV_VIC;
                                MapWriteDestination[0xD4+i] = MV_SID;
                                MapWriteDestination[0xD8+i] = MV_FARB_RAM;
			}
                        CPUWriteProcTbl[0xDC] = Cia1IOWriteProc;
                        CPUWriteProcTbl[0xDD] = Cia2IOWriteProc;
                        CPUWriteProcTbl[0xDE] = IO1WriteProc;
                        CPUWriteProcTbl[0xDF] = IO2WriteProc;
                        MapWriteDestination[0xDC] = MV_CIA1;
                        MapWriteDestination[0xDD] = MV_CIA2;
                        MapWriteDestination[0xDE] = MV_IO1;
                        MapWriteDestination[0xDF] = MV_IO2;
        }
        else if ( MEMORY_MAP == 9 || MEMORY_MAP == 11 ) {
			/// READ
			for(int i=0;i<32;++i)
			{
                                CPUReadProcTbl[0x80+i] = ReadProcFn<MMU>(&MMU::ReadRam,this);
                                CPUReadProcTbl[0xA0+i] = ReadProcFn<MMU>(&MMU::ReadRam,this);
                                CPUReadProcTbl[0xE0+i] = ReadProcFn<MMU>(&MMU::ReadKernalRom,this);
                                MapReadSource[0x80+i] = MV_RAM;
                                MapReadSource[0xA0+i] = MV_RAM;
                                MapReadSource[0xE0+i] = MV_KERNAL_ROM;
			}
			for(int i=0;i<16;++i)
			{
                                CPUReadProcTbl[0xD0+i] = ReadProcFn<MMU>(&MMU::ReadCharRom,this);
                                MapReadSource[0xD0+i] = MV_CHAR_ROM;
			}

			/// WRITE
			for(int i=0;i<32;++i)
			{
                                CPUWriteProcTbl[0x80+i] = WriteProcFn<MMU>(&MMU::WriteRam,this);
                                CPUWriteProcTbl[0xA0+i] = WriteProcFn<MMU>(&MMU::WriteRam,this);
                                CPUWriteProcTbl[0xE0+i] = WriteProcFn<MMU>(&MMU::WriteRam,this);
                                MapWriteDestination[0x80+i] = MV_RAM;
                                MapWriteDestination[0xA0+i] = MV_RAM;
                                MapWriteDestination[0xE0+i] = MV_RAM;
			}
			for(int i=0;i<16;++i)
			{
                                CPUWriteProcTbl[0xD0+i] = WriteProcFn<MMU>(&MMU::WriteRam,this);
                                MapWriteDestination[0xD0+i] = MV_RAM;
			}
        }
        else if ( MEMORY_MAP == 21 || MEMORY_MAP == 23 || MEMORY_MAP == 20 ) {
			/// READ
			for(int i=0;i<32;++i)
			{
                                CPUReadProcTbl[0x80+i] = ReadProcFn<MMU>(&MMU::ReadRam,this);
                                CPUReadProcTbl[0xA0+i] = ReadProcFn<MMU>(&MMU::ReadRam,this);
                                CPUReadProcTbl[0xE0+i] = ReadProcFn<MMU>(&MMU::ReadRam,this);
                                MapReadSource[0x80+i] = MV_RAM;
                                MapReadSource[0xA0+i] = MV_RAM;
                                MapReadSource[0xE0+i] = MV_RAM;
			}
			for(int i=0;i<4;++i)
			{
                                CPUReadProcTbl[0xD0+i] = VicIOReadProc;
                                CPUReadProcTbl[0xD4+i] = SidIOReadProc;
                                CPUReadProcTbl[0xD8+i] = ReadProcFn<MMU>(&MMU::ReadFarbRam,this);
                                MapReadSource[0xD0+i] = MV_VIC;
                                MapReadSource[0xD4+i] = MV_SID;
                                MapReadSource[0xD8+i] = MV_FARB_RAM;
			}
                        CPUReadProcTbl[0xDC] = Cia1IOReadProc;
                        CPUReadProcTbl[0xDD] = Cia2IOReadProc;
                        CPUReadProcTbl[0xDE] = IO1ReadProc;
                        CPUReadProcTbl[0xDF] = IO2ReadProc;
                        MapReadSource[0xDC] = MV_CIA1;
                        MapReadSource[0xDD] = MV_CIA2;
                        MapReadSource[0xDE] = MV_IO1;
                        MapReadSource[0xDF] = MV_IO2;

			for(int i=0;i<32;++i)
			{
                                CPUWriteProcTbl[0x80+i] = WriteProcFn<MMU>(&MMU::WriteRam,this);
                                CPUWriteProcTbl[0xA0+i] = WriteProcFn<MMU>(&MMU::WriteRam,this);
                                CPUWriteProcTbl[0xE0+i] = WriteProcFn<MMU>(&MMU::WriteRam,this);
                                MapWriteDestination[0x80+i] = MV_RAM;
                                MapWriteDestination[0xA0+i] = MV_RAM;
                                MapWriteDestination[0xE0+i] = MV_RAM;
			}
			for(int i=0;i<4;++i)
			{
                                CPUWriteProcTbl[0xD0+i] = VicIOWriteProc;
                                CPUWriteProcTbl[0xD4+i] = SidIOWriteProc;
                                CPUWriteProcTbl[0xD8+i] = WriteProcFn<MMU>(&MMU::WriteFarbRam,this);
                                MapWriteDestination[0xD0+i] = MV_VIC;
                                MapWriteDestination[0xD4+i] = MV_SID;
                                MapWriteDestination[0xD8+i] = MV_FARB_RAM;
			}
                        CPUWriteProcTbl[0xDC] = Cia1IOWriteProc;
                        CPUWriteProcTbl[0xDD] = Cia2IOWriteProc;
                        CPUWriteProcTbl[0xDE] = IO1WriteProc;
                        CPUWriteProcTbl[0xDF] = IO2WriteProc;
                        MapWriteDestination[0xDC] = MV_CIA1;
                        MapWriteDestination[0xDD] = MV_CIA2;
                        MapWriteDestination[0xDE] = MV_IO1;
                        MapWriteDestination[0xDF] = MV_IO2;
        }
        else if ( MEMORY_MAP == 17 || MEMORY_MAP == 29 ) {
			/// READ
			for(int i=0;i<32;++i)
			{
                                CPUReadProcTbl[0x80+i] = ReadProcFn<MMU>(&MMU::ReadRam,this);
                                CPUReadProcTbl[0xA0+i] = ReadProcFn<MMU>(&MMU::ReadRam,this);
                                CPUReadProcTbl[0xE0+i] = ReadProcFn<MMU>(&MMU::ReadRam,this);
                                MapReadSource[0x80+i] = MV_RAM;
                                MapReadSource[0xA0+i] = MV_RAM;
                                MapReadSource[0xE0+i] = MV_RAM;
			}
			for(int i=0;i<16;++i)
			{
                                CPUReadProcTbl[0xD0+i] = ReadProcFn<MMU>(&MMU::ReadCharRom,this);
                                MapReadSource[0xD0+i] = MV_CHAR_ROM;
			}

			/// WRITE
			for(int i=0;i<32;++i)
			{
                                CPUWriteProcTbl[0x80+i] = WriteProcFn<MMU>(&MMU::WriteRam,this);
                                CPUWriteProcTbl[0xA0+i] = WriteProcFn<MMU>(&MMU::WriteRam,this);
                                CPUWriteProcTbl[0xE0+i] = WriteProcFn<MMU>(&MMU::WriteRam,this);
                                MapWriteDestination[0x80+i] = MV_RAM;
                                MapWriteDestination[0xA0+i] = MV_RAM;
                                MapWriteDestination[0xE0+i] = MV_RAM;
			}
			for(int i=0;i<16;++i)
			{
                                CPUWriteProcTbl[0xD0+i] = WriteProcFn<MMU>(&MMU::WriteRam,this);
                                MapWriteDestination[0xD0+i] = MV_RAM;
			}
        }
        else if ( MEMORY_MAP == 1 || MEMORY_MAP == 5 || MEMORY_MAP == 3 || MEMORY_MAP == 7 || MEMORY_MAP == 0 || MEMORY_MAP == 4 || MEMORY_MAP == 16 ) {
			/// READ
			for(int i=0;i<32;++i)
			{
                                CPUReadProcTbl[0x80+i] = ReadProcFn<MMU>(&MMU::ReadRam,this);
                                CPUReadProcTbl[0xA0+i] = ReadProcFn<MMU>(&MMU::ReadRam,this);
                                CPUReadProcTbl[0xE0+i] = ReadProcFn<MMU>(&MMU::ReadRam,this);
                                MapReadSource[0x80+i] = MV_RAM;
                                MapReadSource[0xA0+i] = MV_RAM;
                                MapReadSource[0xE0+i] = MV_RAM;
			}
			for(int i=0;i<16;++i)
			{
                                CPUReadProcTbl[0xD0+i] = ReadProcFn<MMU>(&MMU::ReadRam,this);
                                MapReadSource[0xD0+i] = MV_RAM;
			}

			/// WRITE
			for(int i=0;i<32;++i)
			{
                                CPUWriteProcTbl[0x80+i] = WriteProcFn<MMU>(&MMU::WriteRam,this);
                                CPUWriteProcTbl[0xA0+i] = WriteProcFn<MMU>(&MMU::WriteRam,this);
                                CPUWriteProcTbl[0xE0+i] = WriteProcFn<MMU>(&MMU::WriteRam,this);
                                MapWriteDestination[0x80+i] = MV_RAM;
                                MapWriteDestination[0xA0+i] = MV_RAM;
                                MapWriteDestination[0xE0+i] = MV_RAM;
			}
			for(int i=0;i<16;++i)
			{
                                CPUWriteProcTbl[0xD0+i] = WriteProcFn<MMU>(&MMU::WriteRam,this);
                                MapWriteDestination[0xD0+i] = MV_RAM;
			}
        }
        else if ( MEMORY_MAP == 2 || MEMORY_MAP == 18 || MEMORY_MAP == 10 || MEMORY_MAP == 6 || MEMORY_MAP == 26 || MEMORY_MAP == 22 || MEMORY_MAP == 14 || MEMORY_MAP == 30 ) {
		/// ULTIMAX Modus !!! ///
			/// READ
                        for(int i=0;i<112;i++)
                        {
                            CPUReadProcTbl[0x10+i] = ReadProcFn<MMU>(&MMU::ReadOpenAdress,this);
                            MapReadSource[0x10+i] = MV_OPEN;
                        }
                        for(int i=0;i<16;i++)
                        {
                            CPUReadProcTbl[0xC0+i] = ReadProcFn<MMU>(&MMU::ReadOpenAdress,this);
                            MapReadSource[0xC0+i] = MV_OPEN;
                        }
			for(int i=0;i<32;++i)
			{
                                CPUReadProcTbl[0x80+i] = ReadProcFn<MMU>(&MMU::ReadCRT1,this);
                                CPUReadProcTbl[0xA0+i] = ReadProcFn<MMU>(&MMU::ReadOpenAdress,this);
                                CPUReadProcTbl[0xE0+i] = ReadProcFn<MMU>(&MMU::ReadCRT3,this);
                                MapReadSource[0x80+i] = MV_CRT_1;
                                MapReadSource[0xA0+i] = MV_OPEN;
                                MapReadSource[0xE0+i] = MV_CRT_3;
			}
			for(int i=0;i<4;++i)
			{
                                CPUReadProcTbl[0xD0+i] = VicIOReadProc;
                                CPUReadProcTbl[0xD4+i] = SidIOReadProc;
                                CPUReadProcTbl[0xD8+i] = ReadProcFn<MMU>(&MMU::ReadFarbRam,this);
                                MapReadSource[0xD0+i] = MV_VIC;
                                MapReadSource[0xD4+i] = MV_SID;
                                MapReadSource[0xD8+i] = MV_FARB_RAM;
			}
                        CPUReadProcTbl[0xDC] = Cia1IOReadProc;
                        CPUReadProcTbl[0xDD] = Cia2IOReadProc;
                        CPUReadProcTbl[0xDE] = IO1ReadProc;
                        CPUReadProcTbl[0xDF] = IO2ReadProc;
                        MapReadSource[0xDC] = MV_CIA1;
                        MapReadSource[0xDD] = MV_CIA2;
                        MapReadSource[0xDE] = MV_IO1;
                        MapReadSource[0xDF] = MV_IO2;

			/// WRITE
			for(int i=0;i<112;i++)
			{
                                CPUWriteProcTbl[0x10+i] = WriteProcFn<MMU>(&MMU::WriteOpenAdress,this);
                                MapWriteDestination[0x10+i] = MV_OPEN;
			}
			for(int i=0;i<16;i++)
			{
                                CPUWriteProcTbl[0xC0+i] = WriteProcFn<MMU>(&MMU::WriteOpenAdress,this);
                                MapWriteDestination[0xC0+i] = MV_OPEN;
			}
			for(int i=0;i<32;++i)
			{
                                CPUWriteProcTbl[0x80+i] = WriteProcFn<MMU>(&MMU::WriteCRT1,this);
                                CPUWriteProcTbl[0xA0+i] = WriteProcFn<MMU>(&MMU::WriteOpenAdress,this);
                                CPUWriteProcTbl[0xE0+i] = WriteProcFn<MMU>(&MMU::WriteCRT3,this);
                                MapWriteDestination[0x80+i] = MV_CRT_1;
                                MapWriteDestination[0xA0+i] = MV_OPEN;
                                MapWriteDestination[0xE0+i] = MV_CRT_3;
			}
			for(int i=0;i<4;++i)
			{
                                CPUWriteProcTbl[0xD0+i] = VicIOWriteProc;
                                CPUWriteProcTbl[0xD4+i] = SidIOWriteProc;
                                CPUWriteProcTbl[0xD8+i] = WriteProcFn<MMU>(&MMU::WriteFarbRam,this);
                                MapWriteDestination[0xD0+i] = MV_VIC;
                                MapWriteDestination[0xD4+i] = MV_SID;
                                MapWriteDestination[0xD8+i] = MV_FARB_RAM;
			}
                        CPUWriteProcTbl[0xDC] = Cia1IOWriteProc;
                        CPUWriteProcTbl[0xDD] = Cia2IOWriteProc;
                        CPUWriteProcTbl[0xDE] = IO1WriteProc;
                        CPUWriteProcTbl[0xDF] = IO2WriteProc;
                        MapWriteDestination[0xDC] = MV_CIA1;
                        MapWriteDestination[0xDD] = MV_CIA2;
                        MapWriteDestination[0xDE] = MV_IO1;
                        MapWriteDestination[0xDF] = MV_IO2;
	}
        MEMORY_MAP_OLD = MEMORY_MAP;
}

inline void MMU::InitProcTables(void)
{
        CPUReadProcTbl[0] = ReadProcFn<MMU>(&MMU::ReadZeroPage,this);
        CPUWriteProcTbl[0] = WriteProcFn<MMU>(&MMU::WriteZeroPage,this);
	for (int i = 1; i < 256; ++i)
	{
                CPUReadProcTbl[i] =  ReadProcFn<MMU>(&MMU::ReadRam,this);
                CPUWriteProcTbl[i] = WriteProcFn<MMU>(&MMU::WriteRam,this);
                VICReadProcTbl[i] = ReadProcFn<MMU>(&MMU::ReadVicRam,this);
                MapReadSource[i] = MV_RAM;
                MapWriteDestination[i] = MV_RAM;
	}
	for (int i = 0; i < 16; ++i)
	{
                VICReadProcTbl[0x10+i] = ReadProcFn<MMU>(&MMU::ReadVicCharRomBank0,this);
                VICReadProcTbl[0x90+i] = ReadProcFn<MMU>(&MMU::ReadVicCharRomBank2,this);
	}
}

unsigned char MMU::ReadZeroPage(unsigned short adresse)
{
	if (adresse == 0)
                  return CPU_PORT->DIR_READ;
	if (adresse == 1)
                  return (CPU_PORT->DATA_READ & (0xFF - (((!CPU_PORT->DATA_SET_BIT6)<<6)+((!CPU_PORT->DATA_SET_BIT7)<<7))));
        return RAM[adresse];
}

void MMU::WriteZeroPage(unsigned short adresse, unsigned char wert)
{
    if ( adresse == 0 ) {
        if (CPU_PORT->DATA_SET_BIT7 && ((wert & 0x80) == 0) && CPU_PORT->DATA_FALLOFF_BIT7 == 0)
        {
                        CPU_PORT->DATA_FALLOFF_BIT7 = 1;
        }
        if (CPU_PORT->DATA_SET_BIT6 && ((wert & 0x40) == 0) && CPU_PORT->DATA_FALLOFF_BIT6 == 0)
        {
                        CPU_PORT->DATA_FALLOFF_BIT6 = 1;
        }
        if (CPU_PORT->DATA_SET_BIT7 && (wert & 0x80) && CPU_PORT->DATA_FALLOFF_BIT7)
        {
                        CPU_PORT->DATA_FALLOFF_BIT7 = 0;
        }
        if (CPU_PORT->DATA_SET_BIT6 && (wert & 0x40) && CPU_PORT->DATA_FALLOFF_BIT6)
        {
                        CPU_PORT->DATA_FALLOFF_BIT6 = 0;
        }
        if (CPU_PORT->DIR != wert)
	{
                CPU_PORT->DIR = wert;
                ChangeMemMap();
        }
    }
    else if ( adresse == 1 ) {
        if ((CPU_PORT->DIR & 0x80) && (wert & 0x80))
        {
                        CPU_PORT->DATA_SET_BIT7 = 1;
        }
        if ((CPU_PORT->DIR & 0x40) && (wert & 0x40))
        {
                        CPU_PORT->DATA_SET_BIT6 = 1;
        }

        if (CPU_PORT->DATA != wert)
        {
                          CPU_PORT->DATA = wert;
                          ChangeMemMap();
        }
    } else 
                RAM[adresse] = wert;
}

unsigned char MMU::ReadBasicRom(unsigned short adresse)
{
        return BASIC_ROM[adresse-0xA000];
}

unsigned char MMU::ReadKernalRom(unsigned short adresse)
{
        return KERNAL_ROM[adresse-0xE000];
}

unsigned char MMU::ReadCharRom(unsigned short adresse)
{
        return CHAR_ROM[adresse-0xD000];
}

unsigned char MMU::ReadRam(unsigned short adresse)
{
        return RAM[adresse];
}

void MMU::WriteRam(unsigned short adresse, unsigned char wert)
{
        RAM[adresse] = wert;
}

unsigned char MMU::ReadFarbRam(unsigned short adresse)
{
        return FARB_RAM[adresse-0xD800];
}

void MMU::WriteFarbRam(unsigned short adresse, unsigned char wert)
{
        FARB_RAM[adresse-0xD800] = wert;
}

unsigned char MMU::ReadCRT1(unsigned short adresse)
{
        return CRTRom1ReadProc(adresse);
}

void MMU::WriteCRT1(unsigned short adresse, unsigned char wert)
{
        CRTRom1WriteProc(adresse,wert);
}

unsigned char MMU::ReadCRT2(unsigned short adresse)
{
        return CRTRom2ReadProc(adresse);
}

void MMU::WriteCRT2(unsigned short adresse, unsigned char wert)
{
        CRTRom2WriteProc(adresse,wert);
}

unsigned char MMU::ReadCRT3(unsigned short adresse)
{
        return CRTRom3ReadProc(adresse);
}

void MMU::WriteCRT3(unsigned short adresse, unsigned char wert)
{
        CRTRom3WriteProc(adresse,wert);
}

unsigned char MMU::ReadVicCharRomBank0(unsigned short adresse)
{
        if(*EasyFlashDirty1)
	{
                *EasyFlashDirty1 = false;
                return *EasyFlashByte1;
	}
        return CHAR_ROM[adresse-0x1000];
}

unsigned char MMU::ReadVicCharRomBank2(unsigned short adresse)
{
        if(*EasyFlashDirty2         )
	{
                *EasyFlashDirty2 = false;
                return *EasyFlashByte2;
	}
        return CHAR_ROM[adresse-0x9000];
}

unsigned char MMU::ReadVicRam(unsigned short adresse)
{
        if ( MEMORY_MAP == 2 || MEMORY_MAP == 18 || MEMORY_MAP == 10 || MEMORY_MAP == 6 || MEMORY_MAP == 26 || MEMORY_MAP == 22 || MEMORY_MAP == 14 || MEMORY_MAP == 30 )
	{
                if(adresse >= 0xE000) return CRTRom3ReadProc(adresse);
	}
        return RAM[adresse];
}

unsigned char MMU::ReadOpenAdress(unsigned short)
{
	return 0x88;	// evtl. auf Zufallszahlen !? (steht eigtl. in der Doku)
}

void MMU::WriteOpenAdress(unsigned short, unsigned char)
{
	return;
}

void MMU::InitRam(uint8_t init_value, uint16_t invert_value_every, uint16_t random_pattern_legth, uint16_t repeat_random_pattern, uint16_t random_chance)
{
	uint8_t a,b,value;
	for(int i=0; i <= 0xffff; i++)
	{
		value = init_value;
		a = b = 0;
		if((i / invert_value_every) & 1) value ^= 0xff;
		if((i % repeat_random_pattern) < random_pattern_legth) a = rand() % 256;

		if (random_chance)
		{
			uint8_t mask = 0x80;
			for(int j=0; j<8; j++)
			{
				if(rand() % 1001 < random_chance) b |= mask;
				mask >>= 1;
			}
		}
		RAM[i] = value ^ a ^ b;
	}
}
