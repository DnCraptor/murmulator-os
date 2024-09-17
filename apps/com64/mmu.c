#include "mmu.h"
#include "structs.h"
#include "roms.h"

static void MMU_MMU(MMU* p, MOS6510_PORT* port) {
    p->CPU_PORT = port;
    MMU_InitProcTables(p);
}

static void MMU_Reset(MMU* p)
{
	MOS6510_PORT_Reset(p->CPU_PORT);
	MMU_ChangeMemMap(p);
	MMU_InitRam(p, 0xff, 64, 1, 256, 0); // Funktioniert mit allen VICE RamPattern Tests
}

static unsigned char MMU_ReadRam(MMU* p, unsigned short adresse)
{
    return p->RAM[adresse];
}

static void MMU_WriteRam(MMU* p, unsigned short adresse, unsigned char wert)
{
    p->RAM[adresse] = wert;
}

static unsigned char MMU_ReadFarbRam(MMU* p, unsigned short adresse)
{
    return p->FARB_RAM[adresse];
}

static void MMU_WriteFarbRam(MMU* p, unsigned short adresse, unsigned char wert)
{
    p->FARB_RAM[adresse] = wert;
}

static void MMU_InitProcTables(MMU* p) {
    p->CPUReadProcTbl[0].fn = MMU_ReadZeroPage;
    p->CPUReadProcTbl[0].p  = p;
    p->CPUWriteProcTbl[0].fn = MMU_WriteZeroPage;
    p->CPUWriteProcTbl[0].p  = p;
    p->MapReadSource[0] = MV_RAM;
    p->MapWriteDestination[0] = MV_RAM;
	for(int i = 1; i < 256; ++i)
	{
        p->CPUReadProcTbl[i].fn = MMU_ReadRam;
        p->CPUReadProcTbl[i].p  = p;
        p->CPUWriteProcTbl[i].fn = MMU_WriteRam;
        p->CPUWriteProcTbl[i].p  = p;
        p->MapReadSource[i] = MV_RAM;
        p->MapWriteDestination[i] = MV_RAM;
	}
}

static unsigned char MMU_ReadZeroPage(MMU* p, unsigned short adresse)
{
	switch (adresse) 
	{
      case 0:
        return p->CPU_PORT->DIR_READ;
      case 1:
        return (p->CPU_PORT->DATA_READ & (0xFF - (((!p->CPU_PORT->DATA_SET_BIT6)<<6)+((!p->CPU_PORT->DATA_SET_BIT7)<<7))));
      default:
        return p->RAM[adresse];
	}
}

static void MMU_WriteZeroPage(MMU* p, unsigned short adresse, unsigned char wert)
{
    switch (adresse) 
	{
      case 0:
        if (p->CPU_PORT->DATA_SET_BIT7 && ((wert & 0x80) == 0) && p->CPU_PORT->DATA_FALLOFF_BIT7 == 0)
        {
            p->CPU_PORT->DATA_FALLOFF_BIT7 = 1;
        }
        if (p->CPU_PORT->DATA_SET_BIT6 && ((wert & 0x40) == 0) && p->CPU_PORT->DATA_FALLOFF_BIT6 == 0)
        {
            p->CPU_PORT->DATA_FALLOFF_BIT6 = 1;
        }
        if (p->CPU_PORT->DATA_SET_BIT7 && (wert & 0x80) && p->CPU_PORT->DATA_FALLOFF_BIT7)
        {
            p->CPU_PORT->DATA_FALLOFF_BIT7 = 0;
        }
        if (p->CPU_PORT->DATA_SET_BIT6 && (wert & 0x40) && p->CPU_PORT->DATA_FALLOFF_BIT6)
        {
            p->CPU_PORT->DATA_FALLOFF_BIT6 = 0;
        }
        if (p->CPU_PORT->DIR != wert)
		{
            p->CPU_PORT->DIR = wert;
            MMU_ChangeMemMap(p);
        }
        break;
      case 1:
        if ((p->CPU_PORT->DIR & 0x80) && (wert & 0x80))
        {
            p->CPU_PORT->DATA_SET_BIT7 = 1;
        }
        if ((p->CPU_PORT->DIR & 0x40) && (wert & 0x40))
        {
            p->CPU_PORT->DATA_SET_BIT6 = 1;
        }
        if (p->CPU_PORT->DATA != wert)
		{
            p->CPU_PORT->DATA = wert;
            MMU_ChangeMemMap(p);
        }
        break;
	default:
        p->RAM[adresse] = wert;
    }
}

static void MMU_ChangeMemMap(MMU* p)
{
    MOS6510_PORT_ConfigChanged(p->CPU_PORT, p->CPU_PORT->TAPE_SENSE, 1, 0x17);
    unsigned char AE = ((~p->CPU_PORT->DIR | p->CPU_PORT->DATA) & 0x7);

    p->MEMORY_MAP = 0;
    if (*p->EXROM) p->MEMORY_MAP|=2;
    if (*p->GAME)  p->MEMORY_MAP|=1;
    if ((AE & 4)==4) p->MEMORY_MAP|=4;
    if ((AE & 2)==2) p->MEMORY_MAP|=8;
    if ((AE & 1)==1) p->MEMORY_MAP|=16;

    if(p->MEMORY_MAP == p->MEMORY_MAP_OLD) return;
    
    printf("MEMORY_MAP: %d -> %d\n", p->MEMORY_MAP_OLD, p->MEMORY_MAP);

	switch(p->MEMORY_MAP_OLD)
	{
		case 2: case 18: case 10: case 6: case 26: case 22: case 14: case 30:
			for(int i = 0; i < 112; ++i)
			{
                p->CPUReadProcTbl[0x10 + i].fn = MMU_ReadRam;
                p->CPUReadProcTbl[0x10 + i].p = p;
                p->CPUWriteProcTbl[0x10 + i].fn = MMU_WriteRam;
                p->CPUWriteProcTbl[0x10 + i].p = p;
                p->MapReadSource[0x10 + i] = MV_RAM;
                p->MapWriteDestination[0x10 + i] = MV_RAM;

			}
			for(int i=0;i<16;i++)
			{
                p->CPUReadProcTbl[0xC0 + i].fn = MMU_ReadRam;
                p->CPUReadProcTbl[0xC0 + i].p = p;
                p->CPUWriteProcTbl[0xC0 + i].fn = MMU_WriteRam;
                p->CPUWriteProcTbl[0xC0 + i].p = p;
                p->MapReadSource[0xC0 + i] = MV_RAM;
                p->MapWriteDestination[0xC0 + i] = MV_RAM;
			}
			break;
	}

    switch (p->MEMORY_MAP)
	{
		case 31:
		{
			/// READ
			for(int i = 0; i < 32; ++i)
			{
                p->CPUReadProcTbl[0x80 + i].fn = MMU_ReadRam;
                p->CPUReadProcTbl[0x80 + i].p = p;
                p->CPUReadProcTbl[0xA0 + i].fn = MMU_ReadBasicRom;
                p->CPUReadProcTbl[0xA0 + i].p = p;
                p->CPUReadProcTbl[0xE0 + i].fn = MMU_ReadKernalRom;
                p->CPUReadProcTbl[0xE0 + i].p = p;
                p->MapReadSource[0x80 + i] = MV_RAM;
                p->MapReadSource[0xA0 + i] = MV_BASIC_ROM;
                p->MapReadSource[0xE0 + i] = MV_KERNAL_ROM;
			}
            /**
			for(int i=0;i<4;++i)
			{
                                CPUReadProcTbl[0xD0+i] = VicIOReadProc;
                                CPUReadProcTbl[0xD4+i] = SidIOReadProc;
                                CPUReadProcTbl[0xD8+i] = std::bind(&MMU::ReadFarbRam,this,std::placeholders::_1);
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
            */
			/// WRITE
			for(int i = 0; i < 32; ++i)
			{
                p->CPUWriteProcTbl[0x80 + i].fn = MMU_WriteRam;
                p->CPUWriteProcTbl[0x80 + i].p = p;
                p->CPUWriteProcTbl[0xA0 + i].fn = MMU_WriteRam;
                p->CPUWriteProcTbl[0xA0 + i].p = p;
                p->CPUWriteProcTbl[0xE0 + i].fn = MMU_WriteRam;
                p->CPUWriteProcTbl[0xE0 + i].p = p;
                p->MapWriteDestination[0x80+i] = MV_RAM;
                p->MapWriteDestination[0xA0+i] = MV_RAM;
                p->MapWriteDestination[0xE0+i] = MV_RAM;
			}
            /***
			for(int i=0;i<4;++i)
			{
                                CPUWriteProcTbl[0xD0+i] = VicIOWriteProc;
                                CPUWriteProcTbl[0xD4+i] = SidIOWriteProc;
                                CPUWriteProcTbl[0xD8+i] = std::bind(&MMU::WriteFarbRam,this,std::placeholders::_1,std::placeholders::_2);
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
            */
		}
        break;
        /**
		case 27:
		{
			/// READ
			for(int i=0;i<32;++i)
			{
                                CPUReadProcTbl[0x80+i] = std::bind(&MMU::ReadRam,this,std::placeholders::_1);
                                CPUReadProcTbl[0xA0+i] = std::bind(&MMU::ReadBasicRom,this,std::placeholders::_1);
                                CPUReadProcTbl[0xE0+i] = std::bind(&MMU::ReadKernalRom,this,std::placeholders::_1);
                                MapReadSource[0x80+i] = MV_RAM;
                                MapReadSource[0xA0+i] = MV_BASIC_ROM;
                                MapReadSource[0xE0+i] = MV_KERNAL_ROM;
			}
			for(int i=0;i<16;++i)
			{
                                CPUReadProcTbl[0xD0+i] = std::bind(&MMU::ReadCharRom,this,std::placeholders::_1);
                                MapReadSource[0xD0+i] = MV_CHAR_ROM;
			}

			/// WRITE
			for(int i=0;i<32;++i)
			{
                                CPUWriteProcTbl[0x80+i] = std::bind(&MMU::WriteRam,this,std::placeholders::_1,std::placeholders::_2);
                                CPUWriteProcTbl[0xA0+i] = std::bind(&MMU::WriteRam,this,std::placeholders::_1,std::placeholders::_2);
                                CPUWriteProcTbl[0xE0+i] = std::bind(&MMU::WriteRam,this,std::placeholders::_1,std::placeholders::_2);
                                MapWriteDestination[0x80+i] = MV_RAM;
                                MapWriteDestination[0xA0+i] = MV_RAM;
                                MapWriteDestination[0xE0+i] = MV_RAM;
			}
			for(int i=0;i<16;++i)
			{
                                CPUWriteProcTbl[0xD0+i] = std::bind(&MMU::WriteRam,this,std::placeholders::_1,std::placeholders::_2);
                                MapWriteDestination[0xD0+i] = MV_RAM;
			}
		}break;

		case 29:
		{
			/// READ
			for(int i=0;i<32;++i)
			{
                                CPUReadProcTbl[0x80+i] = std::bind(&MMU::ReadCRT1,this,std::placeholders::_1);
                                CPUReadProcTbl[0xA0+i] = std::bind(&MMU::ReadBasicRom,this,std::placeholders::_1);
                                CPUReadProcTbl[0xE0+i] = std::bind(&MMU::ReadKernalRom,this,std::placeholders::_1);
                                MapReadSource[0x80+i] = MV_CRT_1;
                                MapReadSource[0xA0+i] = MV_BASIC_ROM;
                                MapReadSource[0xE0+i] = MV_KERNAL_ROM;
			}
			for(int i=0;i<4;++i)
			{
                                CPUReadProcTbl[0xD0+i] = VicIOReadProc;
                                CPUReadProcTbl[0xD4+i] = SidIOReadProc;
                                CPUReadProcTbl[0xD8+i] = std::bind(&MMU::ReadFarbRam,this,std::placeholders::_1);
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
                                CPUWriteProcTbl[0x80+i] = std::bind(&MMU::WriteCRT1,this,std::placeholders::_1,std::placeholders::_2);
                                CPUWriteProcTbl[0xA0+i] = std::bind(&MMU::WriteRam,this,std::placeholders::_1,std::placeholders::_2);
                                CPUWriteProcTbl[0xE0+i] = std::bind(&MMU::WriteRam,this,std::placeholders::_1,std::placeholders::_2);
                                MapWriteDestination[0x80+i] = MV_CRT_1;
                                MapWriteDestination[0xA0+i] = MV_RAM;
                                MapWriteDestination[0xE0+i] = MV_RAM;
			}
			for(int i=0;i<4;++i)
			{
                                CPUWriteProcTbl[0xD0+i] = VicIOWriteProc;
                                CPUWriteProcTbl[0xD4+i] = SidIOWriteProc;
                                CPUWriteProcTbl[0xD8+i] = std::bind(&MMU::WriteFarbRam,this,std::placeholders::_1,std::placeholders::_2);
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
		}break;

		case 25:
		{
			/// READ
			for(int i=0;i<32;++i)
			{
                                CPUReadProcTbl[0x80+i] = std::bind(&MMU::ReadCRT1,this,std::placeholders::_1);
                                CPUReadProcTbl[0xA0+i] = std::bind(&MMU::ReadBasicRom,this,std::placeholders::_1);
                                CPUReadProcTbl[0xE0+i] = std::bind(&MMU::ReadKernalRom,this,std::placeholders::_1);
                                MapReadSource[0x80+i] = MV_CRT_1;
                                MapReadSource[0xA0+i] = MV_BASIC_ROM;
                                MapReadSource[0xE0+i] = MV_KERNAL_ROM;
			}
			for(int i=0;i<16;++i)
			{
                                CPUReadProcTbl[0xD0+i] = std::bind(&MMU::ReadCharRom,this,std::placeholders::_1);
                                MapReadSource[0xD0+i] = MV_CHAR_ROM;
			}

			/// WRITE
			for(int i=0;i<32;++i)
			{
                                CPUWriteProcTbl[0x80+i] = std::bind(&MMU::WriteCRT1,this,std::placeholders::_1,std::placeholders::_2);
                                CPUWriteProcTbl[0xA0+i] = std::bind(&MMU::WriteRam,this,std::placeholders::_1,std::placeholders::_2);
                                CPUWriteProcTbl[0xE0+i] = std::bind(&MMU::WriteRam,this,std::placeholders::_1,std::placeholders::_2);
                                MapWriteDestination[0x80+i] = MV_CRT_1;
                                MapWriteDestination[0xA0+i] = MV_RAM;
                                MapWriteDestination[0xE0+i] = MV_RAM;
			}
			for(int i=0;i<16;++i)
			{
                                CPUWriteProcTbl[0xD0+i] = std::bind(&MMU::WriteRam,this,std::placeholders::_1,std::placeholders::_2);
                                MapWriteDestination[0xD0+i] = MV_RAM;
			}
		}break;
*/
		case 28:
		{
			/// READ
			for(int i = 0; i < 32; ++i)
			{
///                                CPUReadProcTbl[0x80+i] = std::bind(&MMU::ReadCRT1,this,std::placeholders::_1);
///                                CPUReadProcTbl[0xA0+i] = std::bind(&MMU::ReadCRT2,this,std::placeholders::_1);
                p->CPUReadProcTbl[0xE0 + i].fn = MMU_ReadKernalRom;
                p->CPUReadProcTbl[0xE0 + i].p = p;
///                                MapReadSource[0x80+i] = MV_CRT_1;
///                                MapReadSource[0xA0+i] = MV_CRT_2;
                p->MapReadSource[0xE0 + i] = MV_KERNAL_ROM;
			}
			for(int i = 0; i < 4; ++i)
			{
///                                CPUReadProcTbl[0xD0+i] = VicIOReadProc;
///                                CPUReadProcTbl[0xD4+i] = SidIOReadProc;
                p->CPUReadProcTbl[0xD8 + i].fn = MMU_ReadFarbRam;
                p->CPUReadProcTbl[0xD8 + i].p = p;
///                                MapReadSource[0xD0+i] = MV_VIC;
///                                MapReadSource[0xD4+i] = MV_SID;
                p->MapReadSource[0xD8 + i] = MV_FARB_RAM;
			}
            /**
                        CPUReadProcTbl[0xDC] = Cia1IOReadProc;
                        CPUReadProcTbl[0xDD] = Cia2IOReadProc;
                        CPUReadProcTbl[0xDE] = IO1ReadProc;
                        CPUReadProcTbl[0xDF] = IO2ReadProc;
                        MapReadSource[0xDC] = MV_CIA1;
                        MapReadSource[0xDD] = MV_CIA2;
                        MapReadSource[0xDE] = MV_IO1;
                        MapReadSource[0xDF] = MV_IO2;
*/
			/// WRITE
			for(int i = 0; i < 32; ++i)
			{
///                                CPUWriteProcTbl[0x80+i] = std::bind(&MMU::WriteCRT1,this,std::placeholders::_1,std::placeholders::_2);
///                                CPUWriteProcTbl[0xA0+i] = std::bind(&MMU::WriteCRT2,this,std::placeholders::_1,std::placeholders::_2);
                p->CPUWriteProcTbl[0xE0 + i].fn = MMU_WriteRam;
                p->CPUWriteProcTbl[0xE0 + i].p = p;
///                                MapWriteDestination[0x80+i] = MV_CRT_1;
///                                MapWriteDestination[0xA0+i] = MV_CRT_2;
                p->MapWriteDestination[0xE0 + i] = MV_RAM;
			}
			for(int i = 0; i < 4; ++i)
			{
///                                CPUWriteProcTbl[0xD0+i] = VicIOWriteProc;
///                                CPUWriteProcTbl[0xD4+i] = SidIOWriteProc;
                p->CPUWriteProcTbl[0xD8 + i].fn = MMU_WriteFarbRam;
                p->CPUWriteProcTbl[0xD8 + i].p = p;
///                                MapWriteDestination[0xD0+i] = MV_VIC;
///                                MapWriteDestination[0xD4+i] = MV_SID;
                p->MapWriteDestination[0xD8 + i] = MV_FARB_RAM;
			}
            /**
                        CPUWriteProcTbl[0xDC] = Cia1IOWriteProc;
                        CPUWriteProcTbl[0xDD] = Cia2IOWriteProc;
                        CPUWriteProcTbl[0xDE] = IO1WriteProc;
                        CPUWriteProcTbl[0xDF] = IO2WriteProc;
                        MapWriteDestination[0xDC] = MV_CIA1;
                        MapWriteDestination[0xDD] = MV_CIA2;
                        MapWriteDestination[0xDE] = MV_IO1;
                        MapWriteDestination[0xDF] = MV_IO2;
            */
		}
        break;
/**
		case 24:
		{
			/// READ
			for(int i=0;i<32;++i)
			{
                                CPUReadProcTbl[0x80+i] = std::bind(&MMU::ReadCRT1,this,std::placeholders::_1);
                                CPUReadProcTbl[0xA0+i] = std::bind(&MMU::ReadCRT2,this,std::placeholders::_1);
                                CPUReadProcTbl[0xE0+i] = std::bind(&MMU::ReadKernalRom,this,std::placeholders::_1);
                                MapReadSource[0x80+i] = MV_CRT_1;
                                MapReadSource[0xA0+i] = MV_CRT_2;
                                MapReadSource[0xE0+i] = MV_KERNAL_ROM;
			}
			for(int i=0;i<16;++i)
			{
                                CPUReadProcTbl[0xD0+i] = std::bind(&MMU::ReadCharRom,this,std::placeholders::_1);
                                MapReadSource[0xD0+i] = MV_CHAR_ROM;
			}

			/// WRITE
			for(int i=0;i<32;++i)
			{
                                CPUWriteProcTbl[0x80+i] = std::bind(&MMU::WriteCRT1,this,std::placeholders::_1,std::placeholders::_2);
                                CPUWriteProcTbl[0xA0+i] = std::bind(&MMU::WriteCRT2,this,std::placeholders::_1,std::placeholders::_2);
                                CPUWriteProcTbl[0xE0+i] = std::bind(&MMU::WriteRam,this,std::placeholders::_1,std::placeholders::_2);
                                MapWriteDestination[0x80+i] = MV_CRT_1;
                                MapWriteDestination[0xA0+i] = MV_CRT_2;
                                MapWriteDestination[0xE0+i] = MV_RAM;
			}
			for(int i=0;i<16;++i)
			{
                                CPUWriteProcTbl[0xD0+i] = std::bind(&MMU::WriteRam,this,std::placeholders::_1,std::placeholders::_2);
                                MapWriteDestination[0xD0+i] = MV_RAM;
			}
		}break;

		case 12:
		{
			/// READ
			for(int i=0;i<32;++i)
			{
                                CPUReadProcTbl[0x80+i] = std::bind(&MMU::ReadRam,this,std::placeholders::_1);
                                CPUReadProcTbl[0xA0+i] = std::bind(&MMU::ReadCRT2,this,std::placeholders::_1);
                                CPUReadProcTbl[0xE0+i] = std::bind(&MMU::ReadKernalRom,this,std::placeholders::_1);
                                MapReadSource[0x80+i] = MV_RAM;
                                MapReadSource[0xA0+i] = MV_CRT_2;
                                MapReadSource[0xE0+i] = MV_KERNAL_ROM;
			}
			for(int i=0;i<4;++i)
			{
                                CPUReadProcTbl[0xD0+i] = VicIOReadProc;
                                CPUReadProcTbl[0xD4+i] = SidIOReadProc;
                                CPUReadProcTbl[0xD8+i] = std::bind(&MMU::ReadFarbRam,this,std::placeholders::_1);
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
                                CPUWriteProcTbl[0x80+i] = std::bind(&MMU::WriteRam,this,std::placeholders::_1,std::placeholders::_2);
                                CPUWriteProcTbl[0xA0+i] = std::bind(&MMU::WriteCRT2,this,std::placeholders::_1,std::placeholders::_2);
                                CPUWriteProcTbl[0xE0+i] = std::bind(&MMU::WriteRam,this,std::placeholders::_1,std::placeholders::_2);
                                MapWriteDestination[0x80+i] = MV_RAM;
                                MapWriteDestination[0xA0+i] = MV_CRT_2;
                                MapWriteDestination[0xE0+i] = MV_RAM;
			}
			for(int i=0;i<4;++i)
			{
                                CPUWriteProcTbl[0xD0+i] = VicIOWriteProc;
                                CPUWriteProcTbl[0xD4+i] = SidIOWriteProc;
                                CPUWriteProcTbl[0xD8+i] = std::bind(&MMU::WriteFarbRam,this,std::placeholders::_1,std::placeholders::_2);
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
		}break;

		case 8:
		{
			/// READ
			for(int i=0;i<32;++i)
			{
                                CPUReadProcTbl[0x80+i] = std::bind(&MMU::ReadRam,this,std::placeholders::_1);
                                CPUReadProcTbl[0xA0+i] = std::bind(&MMU::ReadCRT2,this,std::placeholders::_1);
                                CPUReadProcTbl[0xE0+i] = std::bind(&MMU::ReadKernalRom,this,std::placeholders::_1);
                                MapReadSource[0x80+i] = MV_RAM;
                                MapReadSource[0xA0+i] = MV_CRT_2;
                                MapReadSource[0xE0+i] = MV_KERNAL_ROM;
			}
			for(int i=0;i<16;++i)
			{
                                CPUReadProcTbl[0xD0+i] = std::bind(&MMU::ReadCharRom,this,std::placeholders::_1);
                                MapReadSource[0xD0+i] = MV_CHAR_ROM;
			}

			/// WRITE

			for(int i=0;i<32;++i)
			{
                                CPUWriteProcTbl[0x80+i] = std::bind(&MMU::WriteRam,this,std::placeholders::_1,std::placeholders::_2);
                                CPUWriteProcTbl[0xA0+i] = std::bind(&MMU::WriteCRT2,this,std::placeholders::_1,std::placeholders::_2);
                                CPUWriteProcTbl[0xE0+i] = std::bind(&MMU::WriteRam,this,std::placeholders::_1,std::placeholders::_2);
                                MapWriteDestination[0x80+i] = MV_RAM;
                                MapWriteDestination[0xA0+i] = MV_CRT_2;
                                MapWriteDestination[0xE0+i] = MV_RAM;
			}
			for(int i=0;i<16;++i)
			{
                                CPUWriteProcTbl[0xD0+i] = std::bind(&MMU::WriteRam,this,std::placeholders::_1,std::placeholders::_2);
                                MapWriteDestination[0xD0+i] = MV_RAM;
			}
		}break;

		case 13: case 15:
		{
			/// READ
			for(int i=0;i<32;++i)
			{
                                CPUReadProcTbl[0x80+i] = std::bind(&MMU::ReadRam,this,std::placeholders::_1);
                                CPUReadProcTbl[0xA0+i] = std::bind(&MMU::ReadRam,this,std::placeholders::_1);
                                CPUReadProcTbl[0xE0+i] = std::bind(&MMU::ReadKernalRom,this,std::placeholders::_1);
                                MapReadSource[0x80+i] = MV_RAM;
                                MapReadSource[0xA0+i] = MV_RAM;
                                MapReadSource[0xE0+i] = MV_KERNAL_ROM;
			}
			for(int i=0;i<4;++i)
			{
                                CPUReadProcTbl[0xD0+i] = VicIOReadProc;
                                CPUReadProcTbl[0xD4+i] = SidIOReadProc;
                                CPUReadProcTbl[0xD8+i] = std::bind(&MMU::ReadFarbRam,this,std::placeholders::_1);
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
                                CPUWriteProcTbl[0x80+i] = std::bind(&MMU::WriteRam,this,std::placeholders::_1,std::placeholders::_2);
                                CPUWriteProcTbl[0xA0+i] = std::bind(&MMU::WriteRam,this,std::placeholders::_1,std::placeholders::_2);
                                CPUWriteProcTbl[0xE0+i] = std::bind(&MMU::WriteRam,this,std::placeholders::_1,std::placeholders::_2);
                                MapWriteDestination[0x80+i] = MV_RAM;
                                MapWriteDestination[0xA0+i] = MV_RAM;
                                MapWriteDestination[0xE0+i] = MV_RAM;
			}
			for(int i=0;i<4;++i)
			{
                                CPUWriteProcTbl[0xD0+i] = VicIOWriteProc;
                                CPUWriteProcTbl[0xD4+i] = SidIOWriteProc;
                                CPUWriteProcTbl[0xD8+i] = std::bind(&MMU::WriteFarbRam,this,std::placeholders::_1,std::placeholders::_2);
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
		}break;

		case 9: case 11:
		{
			/// READ
			for(int i=0;i<32;++i)
			{
                                CPUReadProcTbl[0x80+i] = std::bind(&MMU::ReadRam,this,std::placeholders::_1);
                                CPUReadProcTbl[0xA0+i] = std::bind(&MMU::ReadRam,this,std::placeholders::_1);
                                CPUReadProcTbl[0xE0+i] = std::bind(&MMU::ReadKernalRom,this,std::placeholders::_1);
                                MapReadSource[0x80+i] = MV_RAM;
                                MapReadSource[0xA0+i] = MV_RAM;
                                MapReadSource[0xE0+i] = MV_KERNAL_ROM;
			}
			for(int i=0;i<16;++i)
			{
                                CPUReadProcTbl[0xD0+i] = std::bind(&MMU::ReadCharRom,this,std::placeholders::_1);
                                MapReadSource[0xD0+i] = MV_CHAR_ROM;
			}

			/// WRITE
			for(int i=0;i<32;++i)
			{
                                CPUWriteProcTbl[0x80+i] = std::bind(&MMU::WriteRam,this,std::placeholders::_1,std::placeholders::_2);
                                CPUWriteProcTbl[0xA0+i] = std::bind(&MMU::WriteRam,this,std::placeholders::_1,std::placeholders::_2);
                                CPUWriteProcTbl[0xE0+i] = std::bind(&MMU::WriteRam,this,std::placeholders::_1,std::placeholders::_2);
                                MapWriteDestination[0x80+i] = MV_RAM;
                                MapWriteDestination[0xA0+i] = MV_RAM;
                                MapWriteDestination[0xE0+i] = MV_RAM;
			}
			for(int i=0;i<16;++i)
			{
                                CPUWriteProcTbl[0xD0+i] = std::bind(&MMU::WriteRam,this,std::placeholders::_1,std::placeholders::_2);
                                MapWriteDestination[0xD0+i] = MV_RAM;
			}
		}break;

		case 21: case 23: case 20:
		{
			/// READ
			for(int i=0;i<32;++i)
			{
                                CPUReadProcTbl[0x80+i] = std::bind(&MMU::ReadRam,this,std::placeholders::_1);
                                CPUReadProcTbl[0xA0+i] = std::bind(&MMU::ReadRam,this,std::placeholders::_1);
                                CPUReadProcTbl[0xE0+i] = std::bind(&MMU::ReadRam,this,std::placeholders::_1);
                                MapReadSource[0x80+i] = MV_RAM;
                                MapReadSource[0xA0+i] = MV_RAM;
                                MapReadSource[0xE0+i] = MV_RAM;
			}
			for(int i=0;i<4;++i)
			{
                                CPUReadProcTbl[0xD0+i] = VicIOReadProc;
                                CPUReadProcTbl[0xD4+i] = SidIOReadProc;
                                CPUReadProcTbl[0xD8+i] = std::bind(&MMU::ReadFarbRam,this,std::placeholders::_1);
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
                                CPUWriteProcTbl[0x80+i] = std::bind(&MMU::WriteRam,this,std::placeholders::_1,std::placeholders::_2);
                                CPUWriteProcTbl[0xA0+i] = std::bind(&MMU::WriteRam,this,std::placeholders::_1,std::placeholders::_2);
                                CPUWriteProcTbl[0xE0+i] = std::bind(&MMU::WriteRam,this,std::placeholders::_1,std::placeholders::_2);
                                MapWriteDestination[0x80+i] = MV_RAM;
                                MapWriteDestination[0xA0+i] = MV_RAM;
                                MapWriteDestination[0xE0+i] = MV_RAM;
			}
			for(int i=0;i<4;++i)
			{
                                CPUWriteProcTbl[0xD0+i] = VicIOWriteProc;
                                CPUWriteProcTbl[0xD4+i] = SidIOWriteProc;
                                CPUWriteProcTbl[0xD8+i] = std::bind(&MMU::WriteFarbRam,this,std::placeholders::_1,std::placeholders::_2);
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
		}break;

		case 17: case 19:
		{
			/// READ
			for(int i=0;i<32;++i)
			{
                                CPUReadProcTbl[0x80+i] = std::bind(&MMU::ReadRam,this,std::placeholders::_1);
                                CPUReadProcTbl[0xA0+i] = std::bind(&MMU::ReadRam,this,std::placeholders::_1);
                                CPUReadProcTbl[0xE0+i] = std::bind(&MMU::ReadRam,this,std::placeholders::_1);
                                MapReadSource[0x80+i] = MV_RAM;
                                MapReadSource[0xA0+i] = MV_RAM;
                                MapReadSource[0xE0+i] = MV_RAM;
			}
			for(int i=0;i<16;++i)
			{
                                CPUReadProcTbl[0xD0+i] = std::bind(&MMU::ReadCharRom,this,std::placeholders::_1);
                                MapReadSource[0xD0+i] = MV_CHAR_ROM;
			}

			/// WRITE
			for(int i=0;i<32;++i)
			{
                                CPUWriteProcTbl[0x80+i] = std::bind(&MMU::WriteRam,this,std::placeholders::_1,std::placeholders::_2);
                                CPUWriteProcTbl[0xA0+i] = std::bind(&MMU::WriteRam,this,std::placeholders::_1,std::placeholders::_2);
                                CPUWriteProcTbl[0xE0+i] = std::bind(&MMU::WriteRam,this,std::placeholders::_1,std::placeholders::_2);
                                MapWriteDestination[0x80+i] = MV_RAM;
                                MapWriteDestination[0xA0+i] = MV_RAM;
                                MapWriteDestination[0xE0+i] = MV_RAM;
			}
			for(int i=0;i<16;++i)
			{
                                CPUWriteProcTbl[0xD0+i] = std::bind(&MMU::WriteRam,this,std::placeholders::_1,std::placeholders::_2);
                                MapWriteDestination[0xD0+i] = MV_RAM;
			}
		}break;
        */
		case 1: case 5: case 3: case 7: case 0: case 4: case 16:
		{
			/// READ
			for(int i = 0; i < 32; ++i)
			{
                p->CPUReadProcTbl[0x80 + i].fn = MMU_ReadRam;
                p->CPUReadProcTbl[0x80 + i].p = p;
                p->CPUReadProcTbl[0xA0 + i].fn = MMU_ReadRam;
                p->CPUReadProcTbl[0xA0 + i].p = p;
                p->CPUReadProcTbl[0xE0 + i].fn = MMU_ReadRam;
                p->CPUReadProcTbl[0xE0 + i].p = p;
                p->MapReadSource[0x80 + i] = MV_RAM;
                p->MapReadSource[0xA0 + i] = MV_RAM;
                p->MapReadSource[0xE0 + i] = MV_RAM;
			}
			for(int i = 0; i < 16; ++i)
			{
                p->CPUReadProcTbl[0xD0 + i].fn = MMU_ReadRam;
                p->CPUReadProcTbl[0xD0 + i].p = p;
                p->MapReadSource[0xD0 + i] = MV_RAM;
			}
			/// WRITE
			for(int i = 0; i < 32; ++i)
			{
                p->CPUWriteProcTbl[0x80 + i].fn = MMU_WriteRam;
                p->CPUWriteProcTbl[0x80 + i].p = p;
                p->CPUWriteProcTbl[0xA0 + i].fn = MMU_WriteRam;
                p->CPUWriteProcTbl[0xA0 + i].p = p;
                p->CPUWriteProcTbl[0xE0 + i].fn = MMU_WriteRam;
                p->CPUWriteProcTbl[0xE0 + i].p = p;
                p->MapWriteDestination[0x80+i] = MV_RAM;
                p->MapWriteDestination[0xA0+i] = MV_RAM;
                p->MapWriteDestination[0xE0+i] = MV_RAM;
			}
			for(int i=0;i<16;++i)
			{
                p->CPUWriteProcTbl[0xD0 + i].fn = MMU_WriteRam;
                p->CPUWriteProcTbl[0xD0 + i].p = p;
                p->MapWriteDestination[0xD0+i] = MV_RAM;
			}
		}
        break;
        /**
		/// ULTIMAX Modus !!! ///
		case 2: case 18: case 10: case 6: case 26: case 22: case 14: case 30:
		{
			/// READ
                        for(int i=0;i<112;i++)
                        {
                            CPUReadProcTbl[0x10+i] = std::bind(&MMU::ReadOpenAdress,this,std::placeholders::_1);
                            MapReadSource[0x10+i] = MV_OPEN;
                        }
                        for(int i=0;i<16;i++)
                        {
                            CPUReadProcTbl[0xC0+i] = std::bind(&MMU::ReadOpenAdress,this,std::placeholders::_1);
                            MapReadSource[0xC0+i] = MV_OPEN;
                        }
			for(int i=0;i<32;++i)
			{
                                CPUReadProcTbl[0x80+i] = std::bind(&MMU::ReadCRT1,this,std::placeholders::_1);
                                CPUReadProcTbl[0xA0+i] = std::bind(&MMU::ReadOpenAdress,this,std::placeholders::_1);
                                CPUReadProcTbl[0xE0+i] = std::bind(&MMU::ReadCRT3,this,std::placeholders::_1);
                                MapReadSource[0x80+i] = MV_CRT_1;
                                MapReadSource[0xA0+i] = MV_OPEN;
                                MapReadSource[0xE0+i] = MV_CRT_3;
			}
			for(int i=0;i<4;++i)
			{
                                CPUReadProcTbl[0xD0+i] = VicIOReadProc;
                                CPUReadProcTbl[0xD4+i] = SidIOReadProc;
                                CPUReadProcTbl[0xD8+i] = std::bind(&MMU::ReadFarbRam,this,std::placeholders::_1);
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
                                CPUWriteProcTbl[0x10+i] = std::bind(&MMU::WriteOpenAdress,this,std::placeholders::_1,std::placeholders::_2);
                                MapWriteDestination[0x10+i] = MV_OPEN;
			}
			for(int i=0;i<16;i++)
			{
                                CPUWriteProcTbl[0xC0+i] = std::bind(&MMU::WriteOpenAdress,this,std::placeholders::_1,std::placeholders::_2);
                                MapWriteDestination[0xC0+i] = MV_OPEN;
			}
			for(int i=0;i<32;++i)
			{
                                CPUWriteProcTbl[0x80+i] = std::bind(&MMU::WriteCRT1,this,std::placeholders::_1,std::placeholders::_2);
                                CPUWriteProcTbl[0xA0+i] = std::bind(&MMU::WriteOpenAdress,this,std::placeholders::_1,std::placeholders::_2);
                                CPUWriteProcTbl[0xE0+i] = std::bind(&MMU::WriteCRT3,this,std::placeholders::_1,std::placeholders::_2);
                                MapWriteDestination[0x80+i] = MV_CRT_1;
                                MapWriteDestination[0xA0+i] = MV_OPEN;
                                MapWriteDestination[0xE0+i] = MV_CRT_3;
			}
			for(int i=0;i<4;++i)
			{
                                CPUWriteProcTbl[0xD0+i] = VicIOWriteProc;
                                CPUWriteProcTbl[0xD4+i] = SidIOWriteProc;
                                CPUWriteProcTbl[0xD8+i] = std::bind(&MMU::WriteFarbRam,this,std::placeholders::_1,std::placeholders::_2);
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
        break;
        */
	}
    p->MEMORY_MAP_OLD = p->MEMORY_MAP;
}

static unsigned char MMU_ReadBasicRom(MMU* p, unsigned short adresse)
{
    return rom_basic[adresse - 0xA000];
}

static unsigned char MMU_ReadKernalRom(MMU* p, unsigned short adresse)
{
    return rom_kernal[adresse - 0xE000];
}

static void MMU_InitRam(
    MMU* p,
    uint8_t init_value,
    uint16_t invert_value_every,
    uint16_t random_pattern_legth,
    uint16_t repeat_random_pattern,
    uint16_t random_chance
) {
	uint8_t a,b,value;
	for(int i = 0; i <= 0xffff; i++)
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
		p->RAM[i] = value ^ a ^ b;
	}
}
