//////////////////////////////////////////////////
//                                              //
// Emu64                                        //
// von Thorsten Kattanek                        //
//                                              //
// #file) { mos6510_class.cpp                     //
//                                              //
// Dieser Sourcecode ist Copyright geschützt!   //
// Geistiges Eigentum von Th.Kattanek           //
//                                              //
// www.emu64.de                                 //
//                                              //
//////////////////////////////////////////////////

#include "mos6510_class.h"
#include "micro_code_tbl_6510.h"

#define CHK_RDY	if(!*RDY && !CpuWait){CpuWait=true;MCT--;goto L1;}
#define OLD_IRQHandling

MOS6510::MOS6510(void): ReadProcTbl(nullptr), WriteProcTbl(nullptr)
{
	MCT = ((unsigned char*)MicroCodeTable6510 + (0x100*MCTItemSize));
	Pointer = 0;
	Adresse = 0x0000;
	BranchAdresse = 0x0000;
	AktOpcodePC = 0x0100;
	JAMFlag = false;
	TMPByte = 0;
	CpuWait = false;

	PC = 0;
	AC = 0;
	XR = 0;
	YR = 0;
	SP = 0;
	SR = 32;

	EnableExtInterrupts = false;

	irq_state = 0;
	irq_is_low_pegel = false;
	irq_is_active = false;

	irq_delay = false;
	irq_delay_sr_value = 0;

	nmi_state = false;
	nmi_state_old = false;
	nmi_fall_edge = false;
	nmi_is_active = false;

	EnableDebugCart = false;
///	WRITE_DEBUG_CART = false;

	shxy_dma = false;
}

MOS6510::~MOS6510(void)
{
}

static int extInterrupts(bool e, int typ) {
	if(e)
	{
		if(typ == EXT_IRQ)
			typ = CIA_IRQ;
		else if(typ == EXT_NMI)
			typ = CIA_NMI;
		else 
			typ = NOP_INT;
	}
	return typ;
}

void MOS6510::TriggerInterrupt(int typ)
{
	typ = extInterrupts(EnableExtInterrupts, typ);
	if (typ == CIA_IRQ)
			//Interrupts[CIA_IRQ] = true;
			irq_state |= 0x01;
	else if (typ == CIA_NMI) {
			if(Interrupts[CIA_NMI] == true) return;
			Interrupts[CIA_NMI] = true;
			if((Interrupts[CRT_NMI] == false) && (Interrupts[RESTORE_NMI] == false)) nmi_state = true;
	}
	else if (typ == VIC_IRQ)
			//Interrupts[VIC_IRQ] = true;
			irq_state |= 0x02;
	else if (typ == REU_IRQ)
			//Interrupts[REU_IRQ] = true;
			irq_state |= 0x04;
	else if (typ == CRT_NMI) {
			if(Interrupts[CRT_NMI] == true) return;
			Interrupts[CRT_NMI] = true;
			if((Interrupts[CIA_NMI] == false) && (Interrupts[RESTORE_NMI] == false)) nmi_state = true;
	}
	else if (typ == RESTORE_NMI) {
			if(Interrupts[RESTORE_NMI] == true) return;
			Interrupts[RESTORE_NMI] = true;
			if((Interrupts[CIA_NMI] == false) && (Interrupts[CRT_NMI] == false)) nmi_state = true;
	}
}

void MOS6510::ClearInterrupt(int typ)
{
	typ = extInterrupts(EnableExtInterrupts, typ);
	if (typ == CIA_IRQ)
			//Interrupts[CIA_IRQ] = false;
			irq_state &= 0xFE;
	else if (typ == CIA_NMI) {
			Interrupts[CIA_NMI] = false;
			if(Interrupts[CRT_NMI] == false) nmi_state = false;
	}
	else if (typ == VIC_IRQ)
			//Interrupts[VIC_IRQ] = false;
			irq_state &= 0xFD;
	else if (typ == REU_IRQ)
			//Interrupts[REU_IRQ] = false;
			irq_state &= 0xFB;
	else if (typ == CRT_NMI) {
			Interrupts[CRT_NMI] = false;
			if(Interrupts[CIA_NMI] == false) nmi_state = false;
	}
	else if (typ == RESTORE_NMI) {
			Interrupts[RESTORE_NMI] = false;
			if((Interrupts[CIA_NMI] == false) && (Interrupts[CRT_NMI] == false)) nmi_state = false;
	}
}

void MOS6510::ClearJAMFlag(void)
{
	JAMFlag = false;
}

void MOS6510::SetRegister(REG_STRUCT *reg)
{
	if(reg == 0) return;

	unsigned char mask = reg->reg_mask;
	if((mask&1) == 1)
	{
		PC = reg->pc;
		MCT = ((unsigned char*)MicroCodeTable6510 + 6);
	}
	mask>>=1;
	if((mask&1) == 1) AC = (unsigned char)reg->ac;
	mask>>=1;
	if((mask&1) == 1) XR = (unsigned char)reg->xr;
	mask>>=1;
	if((mask&1) == 1) YR = (unsigned char)reg->yr;
	mask>>=1;
	if((mask&1) == 1) SP = (unsigned char)reg->sp;
	mask>>=1;
	if((mask&1) == 1) SR = (unsigned char)reg->sr | 32;
}

bool MOS6510::GetInterrupts(int typ)
{
	return Interrupts[typ];
}

void MOS6510::GetRegister(REG_STRUCT *reg)
{
	if(reg == 0) return;

	unsigned char mask = reg->reg_mask;
	if((mask&1) == 1) reg->pc = PC;
	mask>>=1;
	if((mask&1) == 1) reg->ac = AC;
	mask>>=1;
	if((mask&1) == 1) reg->xr = XR;
	mask>>=1;
	if((mask&1) == 1) reg->yr = YR;
	mask>>=1;
	if((mask&1) == 1) reg->sp = SP;
	mask>>=1;
	if((mask&1) == 1) reg->sr = SR;
	mask>>=1;
	if((mask&1) == 1)
	{
		reg->irq = Read(0xFFFE);
		reg->irq |= Read(0xFFFF)<<8;
	}
	mask>>=1;
	if((mask&1) == 1)
	{
		reg->nmi = Read(0xFFFA);
		reg->nmi |= Read(0xFFFB)<<8;
	}
	reg->_0314 = Read(0x314);
	reg->_0314 |= Read(0x315)<<8;
	reg->_0318 = Read(0x318);
	reg->_0318 |= Read(0x319)<<8;
}

void MOS6510::GetInterneRegister(IREG_STRUCT* ireg)
{
	if(ireg == 0) return;
	ireg->current_opcode_pc = AktOpcodePC;
	ireg->current_opcode = AktOpcode;
	ireg->current_micro_code = *MCT;
	ireg->cpu_wait = CpuWait;
	ireg->jam_flag = JAMFlag;
	ireg->pointer = Pointer;
	ireg->address = Adresse;
	ireg->branch_address = BranchAdresse;
	ireg->tmp_byte = TMPByte;
	ireg->irq = irq_state;
	ireg->nmi = Interrupts[CIA_NMI] | Interrupts[CRT_NMI] | Interrupts[RESTORE_NMI];
	ireg->rdy = *RDY;
	ireg->reset = *RESET;
}

void MOS6510::SetEnableDebugCart(bool enabled)
{
	EnableDebugCart = enabled;
///	WRITE_DEBUG_CART = false;
}

unsigned char MOS6510::GetDebugCartValue()
{
	return DebugCartValue;
}

/*
bool MOS6510::SaveFreez(FILE *File)
{
	unsigned short MCTOffset = MCT-(unsigned char*)MicroCodeTable6510;
	fwrite(&MCTOffset,1,sizeof(MCTOffset),File);
	fwrite(&Pointer,1,sizeof(Pointer),File);
	fwrite(&Adresse,1,sizeof(Adresse),File);
	fwrite(&BranchAdresse,1,sizeof(BranchAdresse),File);
	fwrite(&AktOpcodePC,1,sizeof(AktOpcodePC),File);
	fwrite(&CpuWait,1,sizeof(CpuWait),File);
	fwrite(&JAMFlag,1,sizeof(JAMFlag),File);
	fwrite(&TMPByte,1,sizeof(TMPByte),File);
	fwrite(Interrupts,1,sizeof(Interrupts),File);
	fwrite(&NMIState,1,sizeof(NMIState),File);
	fwrite(&PC,1,sizeof(PC),File);
	fwrite(&AC,1,sizeof(AC),File);
	fwrite(&XR,1,sizeof(XR),File);
	fwrite(&YR,1,sizeof(YR),File);
	fwrite(&SP,1,sizeof(SP),File);
	fwrite(&SR,1,sizeof(SR),File);
	return true;
}

bool MOS6510::LoadFreez(FILE *File,unsigned short Version)
{
	unsigned short MCTOffset;

	switch(Version)
	{
	else if (c == 0x0100) {
	else if (c == 0x0101) {
		fread(&MCTOffset,1,sizeof(MCTOffset),File);
		MCT = MCTOffset + (unsigned char*)MicroCodeTable6510;
		fread(&Pointer,1,sizeof(Pointer),File);
		fread(&Adresse,1,sizeof(Adresse),File);
		fread(&BranchAdresse,1,sizeof(BranchAdresse),File);
		fread(&AktOpcodePC,1,sizeof(AktOpcodePC),File);
		fread(&CpuWait,1,sizeof(CpuWait),File);
		fread(&JAMFlag,1,sizeof(JAMFlag),File);
		fread(&TMPByte,1,sizeof(TMPByte),File);
		fread(Interrupts,1,sizeof(Interrupts),File);
		fread(&NMIState,1,sizeof(NMIState),File);
		fread(&PC,1,sizeof(PC),File);
		fread(&AC,1,sizeof(AC),File);
		fread(&XR,1,sizeof(XR),File);
		fread(&YR,1,sizeof(YR),File);
		fread(&SP,1,sizeof(SP),File);
		fread(&SR,1,sizeof(SR),File);
		break;
	}
	return true;
}
*/

inline void MOS6510::SET_SR_NZ(unsigned char wert)
{
	SR = (SR&0x7D)|(wert&0x80);
	if (wert==0) SR|=2;
}

inline void MOS6510::SET_SIGN(unsigned char wert)
{
	SR = (SR&127)|(wert&0x80);
}

inline void MOS6510::SET_ZERO(unsigned char wert)
{
	if(wert==0) SR|=2;
	else SR&=0xFD;
}

inline void MOS6510::SET_CARRY(unsigned char status)
{
	if (status!=0) SR|=1;
	else SR&=0xFE;
}

inline bool MOS6510::IF_CARRY(void)
{
	if(SR&1) return true;
	return false;
}

inline bool MOS6510::IF_DECIMAL(void)
{
	if(SR&8) return true;
	return false;
}

inline void MOS6510::SET_OVERFLOW(bool status)
{
	if (status!=0) SR|=64;
	else SR&=0xBF;
}

inline unsigned char MOS6510::Read(unsigned short adresse)
{
	unsigned char wert = ReadProcTbl[(adresse)>>8](adresse);
/**
	if(Breakpoints[adresse] & 16)
	{
		*BreakStatus |=16;
		BreakWerte[4] = adresse;
	}

	if(Breakpoints[wert] & 64)
	{
		*BreakStatus |=64;
		BreakWerte[6] = wert;
	}*/
	return wert;
}

inline void MOS6510::Write(unsigned short adresse, unsigned char wert)
{
	if((EnableDebugCart == true) && (adresse == DEBUG_CART_ADRESS))
	{
		DebugCartValue = wert;
///		WRITE_DEBUG_CART = true;
	}

	if(adresse == 0xFF00) WRITE_FF00 = true;
	/**
	if(Breakpoints[adresse] & 32)
	{
		*BreakStatus |=32;
		BreakWerte[5] = adresse;
	}

	if(Breakpoints[wert] & 128)
	{
		*BreakStatus |=128;
		BreakWerte[7] = wert;
	}*/
	WriteProcTbl[(adresse)>>8](adresse,wert);
}

// Phi2
bool MOS6510::OneZyklus(void)
{
	static bool RESET_OLD = false;
	static unsigned char   idxReg = 0;

	unsigned int    tmp;
	unsigned int    tmp1;
	unsigned short  tmp2;
	unsigned short  src;
	bool            carry_tmp;

	if(*RDY) CpuWait = false;

	if(!*RESET)
	{
		CpuWait=true;
	}

	if((*RESET == true) && (RESET_OLD == false))
	{
		CpuWait=false;
		Interrupts[CIA_NMI] = false;
		Interrupts[CRT_NMI] = false;
		Interrupts[RESTORE_NMI] = false;

		JAMFlag = false;
		SR = 0x04;
		MCT = ((unsigned char*)MicroCodeTable6510 + (0x100*MCTItemSize));
		AktOpcode = 0x100;
	}
	RESET_OLD = *RESET;

	// IRQ auf low Pegel prüfen
	if(irq_state > 0)
		irq_is_low_pegel = true;
	else
		irq_is_low_pegel = false;

	// NMI auf fallende Flanke überprüfen
	if(nmi_state == true && nmi_state_old == false)
		nmi_fall_edge = true;
	else
		nmi_fall_edge = false;
	nmi_state_old = nmi_state;

	if(!CpuWait)
	{
		char c = *MCT;
		//R // Feetch Opcode
		if (c == 0) {
			if(JAMFlag) return false;
			CHK_RDY

			if(nmi_is_active == true) // NMIStatePuffer[CYCLES] --> 2 CYCLES Sagt zwei Zyklen vorher muss der NMI schon angelegen haben also vor dem letzten Zyklus des vorigen Befehls
			{
				nmi_is_active = false;
				MCT = ((unsigned char*)MicroCodeTable6510 + (0x102*MCTItemSize));
				AktOpcode = 0x102;

				irq_delay = false;
				return false;
			}
			else
			{
				if((irq_is_active == true) && (irq_delay ? ((irq_delay_sr_value&4)==0) : ((SR&4)==0))) // IRQLinePuffer[CYCLES] --> 2 CYCLES Sagt zwei Zyklen vorher muss der IRQ schon anliegen also vor dem letzten Zyklus des vorigen Befehls
				{
					MCT = ((unsigned char*)MicroCodeTable6510 + (0x101*MCTItemSize));
					AktOpcode = 0x101;

					irq_delay = false;
					return false;
				}
			}

			irq_delay = false;

			MCT = ((unsigned char*)MicroCodeTable6510 + (Read(PC)*MCTItemSize));
			AktOpcode = ReadProcTbl[(AktOpcodePC)>>8](AktOpcodePC);

///			*HistoryPointer = *HistoryPointer+1;
///			History[*HistoryPointer] = AktOpcodePC;

			PC++;

			return false;
		}

		//R // Lesen von PC-Adresse und verwerfen // PC++
		else if (c == 1) {
			CHK_RDY
			Read(PC);
			PC++;
		}
		//W // PC Hi -> Stack // SR|16 // SP--
		else if (c == 2) {
			Write(0x0100+(SP),GetPCHi);
			SR|=16;
			SP--;
		}
		//W // PC Lo -> Stack // SP--
		else if (c == 3) {
			Write(0x0100+(SP),GetPCLo);
			SP--;
		}
		//W // SR -> Stack // SR|4 // SP--
		else if (c == 4) {
			Write(0x0100+(SP),SR);
			SR|=4;
			SP--;
		}
		//R // PC Lo von 0xFFFE holen
		else if (c == 5) {
			CHK_RDY
			SetPCLo(Read(0xFFFE));
		}
		//R // PC Hi von 0xFFFF holen
		else if (c == 6) {
			CHK_RDY
			SetPCHi(Read(0xFFFF));
		}
		//R // Pointer von PC-Adresse holen // PC++
		else if (c == 7) {
			CHK_RDY
			Pointer = Read(PC);
			PC++;
		}
		//R // Lesen von Pointer und verwerfen // Pointer+XR
		else if (c == 8) {
			CHK_RDY
			Read((unsigned short)Pointer);
			Pointer += XR;
		}
		//R // Adresse Lo von Pointer-Adresse holen // Pointer++
		else if (c == 9) {
				CHK_RDY
				SetAdresseLo(Read((unsigned short)Pointer));
				Pointer++;
		}
		//R // Adresse Hi von Pointer-Adresse holen //
		else if (c == 10) {
				CHK_RDY
				SetAdresseHi(Read((unsigned short)Pointer));
		}
		//R // TMPByte von Adresse holen // AC or TMPByte // Set SR(NZ)
		else if (c == 11) {
				CHK_RDY
				TMPByte = Read(Adresse);
				AC |= TMPByte;
				SET_SR_NZ(AC);
		}
		//R // JAM
		else if (c == 12) {
				CHK_RDY
				CpuWait=false;
				SR = 0x04;
				MCT = ((unsigned char*)MicroCodeTable6510 + (0x100*MCTItemSize));
				return 0;
		}
		//R // TMPByte von Adresse holen
		else if (c == 13) {
				// Prüfen ob DMA statt findet
				if(!shxy_dma)
				{
				if(!*RDY && !CpuWait)
					shxy_dma = true;
				else
					shxy_dma = false;
				}

				CHK_RDY
				TMPByte = Read(Adresse);
		}
		//W // TMPByte nach Adresse schreiben // ASL MEMORY // ORA
		else if (c == 14) {
			Write(Adresse,TMPByte);

			SET_CARRY(TMPByte&0x80);	// ASL MEMORY
			TMPByte <<= 1;
			TMPByte &= 0xFF;

			AC|=TMPByte;				// ORA
			SET_SR_NZ(AC);

		}
		//W // TMPByte nach Adresse schreiben
		else if (c == 15) {
				Write(Adresse,TMPByte);
		}
		//R // Adresse Hi = 0 // Adresse Lo von PC-Adresse holen // PC++
		else if (c == 16) {
				CHK_RDY
				Adresse = Read(PC);
				PC++;
		}
		//R // TMPByte von Adresse lesen // Adresse Lo + YR
		else if (c == 17) {
				CHK_RDY
				TMPByte = Read(Adresse);
				Adresse += YR;
				Adresse &= 0xFF;
		}
		//W // TMPByte nach Adresse schreiben // TMPByte<<1 // Set SR(NZC)
		else if (c == 18) {
				Write(Adresse,TMPByte);
				SET_CARRY(TMPByte&0x80);
				TMPByte <<= 1;
				TMPByte &= 0xFF;
				SET_SIGN(TMPByte);
				SET_ZERO(TMPByte);
		}
		//R // TMPByte von PC-Adresse holen
		else if (c == 19) {
				CHK_RDY
				TMPByte = Read(PC);
		}
		//W // SR nach SP+0x0100 schreiben // SP-1
		else if (c == 20) {
				Write(SP+0x0100,SR|16);
				SP--;
		}
		//R // TMPByte von PC-Adresse holen // AC or TMPByte // PC+1
		else if (c == 21) {
				CHK_RDY
				TMPByte = Read(PC);
				AC|=TMPByte;
				SET_SR_NZ(AC);
				PC++;
		}
		//R // TMPByte von PC-Adresse holen // AC<<1 // Set SR(NZC)
		else if (c == 22) {
				CHK_RDY
				TMPByte = Read(PC);
				SET_CARRY(AC&0x80);
				AC <<= 1;
				AC &= 0xFF;
				SET_SIGN(AC);
				SET_ZERO(AC);
		}
		//R // TMPByte von PC-Adresse holen // AC and TMPByte // Set SR(NZC) // PC+1
		else if (c == 23) {
				CHK_RDY
				TMPByte = Read(PC);
				AC&=TMPByte;
				SET_SR_NZ(AC);
				SR&=0xFE;
				SR|=AC>>7;
				PC++;
		}
		//R // Adresse Lo von PC-Adresse holen // PC+1
		else if (c == 24) {
				CHK_RDY
				SetAdresseLo(Read(PC));
				PC++;
		}
		//R // Adresse Hi von PC-Adresse holen // PC+1
		else if (c == 25) {
				CHK_RDY
				SetAdresseHi(Read(PC));
				PC++;
		}
		//R // TMPByte von PC-Adresse holen // PC+1 // SR(N) auf FALSE prÃŒfen (BPL)
		else if (c == 26) {
				CHK_RDY
				TMPByte = Read(PC);
				PC++;
				if((SR&0x80)!=0x00) MCT+=2;
		}
		//R // TMPByte von PC-Adresse holen // PC+1 // SR(N) auf TRUE prÃŒfen (BMI)
		else if (c == 27) {
				CHK_RDY
				TMPByte = Read(PC);
				PC++;
				if((SR&0x80)!=0x80) MCT+=2;
		}
		//R // TMPByte von PC-Adresse holen // PC+1 // SR(V) auf FALSE prÃŒfen (BVC)
		else if (c == 28) {
				CHK_RDY
				TMPByte = Read(PC);
				PC++;
				if((SR&0x40)!=0x00) MCT+=2;
		}
		//R // TMPByte von PC-Adresse holen // PC+1 // SR(V) auf TRUE prÃŒfen (BVS)
		else if (c == 29) {
				CHK_RDY
				TMPByte = Read(PC);
				PC++;
				if((SR&0x40)!=0x40) MCT+=2;
		}
		//R // TMPByte von PC-Adresse holen // PC+1 // SR(C) auf FALSE prÃŒfen (BCC)
		else if (c == 30) {
				CHK_RDY
				TMPByte = Read(PC);
				PC++;
				if((SR&0x01)!=0x00) MCT+=2;
		}
		//R // TMPByte von PC-Adresse holen // PC+1 // SR(C) auf TRUE prÃŒfen (BCS)
		else if (c == 31) {
				CHK_RDY
				TMPByte = Read(PC);
				PC++;
				if((SR&0x01)!=0x01) MCT+=2;
		}
		//R // TMPByte von PC-Adresse holen // PC+1 // SR(Z) auf FALSE prÃŒfen (BNE)
		else if (c == 32) {
				CHK_RDY
				TMPByte = Read(PC);
				PC++;
				if((SR&0x02)!=0x00) MCT+=2;
		}
		//R // TMPByte von PC-Adresse holen // PC+1 // SR(Z) auf TRUE prÃŒfen (BEQ)
		else if (c == 33) {
				CHK_RDY
				TMPByte = Read(PC);
				PC++;
				if((SR&0x02)!=0x02) MCT+=2;
		}
		//R // Lesen von PC-Adresse und verwerfen // BranchAdresse=PC+TMPByte
		else if (c == 34) {
				CHK_RDY
				Read(PC);
				BranchAdresse = PC + (signed char)(TMPByte);
				if ((PC ^ BranchAdresse) & 0xFF00)
				{
						PC = (PC&0xFF00)|(BranchAdresse&0xFF);
				}
				else
				{
						PC = BranchAdresse;
						MCT+=1;
				}
		}
		//R // FIX PC Hi Adresse (Im Branchbefehl)
		else if (c == 35) {
				CHK_RDY
				Read(PC);
				PC = BranchAdresse;
		}
		//R // Adresse Hi von Pointer-Adresse holen // Adresse+YR
		else if (c == 36) {
				CHK_RDY
				SetAdresseHi(Read((unsigned short)Pointer));
				Adresse += YR;
				idxReg = YR;
		}
		//R // TMPByte von Adresse holen // Fix Adresse Hi MCT+1 // AC or TMPByte //
		else if (c == 37) {
				CHK_RDY
				if((Adresse&0xFF) >= idxReg)
				{
						TMPByte = Read(Adresse);
						AC|=TMPByte;
						SET_SR_NZ(AC);
						MCT++;
				}
		}
		//R // Adresse Hi von PC-Adresse holen // PC=Adresse
		else if (c == 38) {
				CHK_RDY
				SetAdresseHi(Read(PC));
				PC = Adresse;
		}
		//R // Lesen von PC-Adresse und verwerfen // XR=AC // Set SR(NZ)
		else if (c == 39) {
				CHK_RDY
				Read(PC);
				XR = AC;
				SET_SR_NZ(XR);
		}
		//R // Lesen von PC-Adresse und verwerfen // YR=AC // Set SR(NZ)
		else if (c == 40) {
				CHK_RDY
				Read(PC);
				YR = AC;
				SET_SR_NZ(YR);
		}
		//R // Lesen von PC-Adresse und verwerfen // XR=SP // Set SR(NZ)
		else if (c == 41) {
				CHK_RDY
				Read(PC);
				XR = SP;
				SET_SR_NZ(XR);
		}
		//R // Lesen von PC-Adresse und verwerfen // AC=XR // Set SR(NZ)
		else if (c == 42) {
				CHK_RDY
				Read(PC);
				AC = XR;
				SET_SR_NZ(AC);
		}
		//R // Lesen von PC-Adresse und verwerfen // SP=XR
		else if (c == 43) {
				CHK_RDY
				Read(PC);
				SP = XR;
		}
		//R // Lesen von PC-Adresse und verwerfen // AC=YR // Set SR(NZ)
		else if (c == 44) {
				CHK_RDY
				Read(PC);
				AC = YR;
				SET_SR_NZ(AC);
		}
		//W // AC nach SP+0x0100 schreiben // SP-1
		else if (c == 45) {
				Write(SP+0x0100,AC);
				SP--;
		}
		//R // AC von SP+0x0100 lesen // SP+1
		else if (c == 46) {
				CHK_RDY
				AC = Read(SP+0x0100);
				SP++;
		}
		//R // AC von SP+0x0100 lesen // Set SR(NZ)
		else if (c == 47) {
				CHK_RDY
				AC = Read(SP+0x0100);
				SET_SR_NZ(AC);
		}
		//R // SR von SP+0x0100 lesen // SP+1
		else if (c == 48) {
				CHK_RDY
				SR = Read(SP+0x0100)|32;
				SP++;
		}
		//R // SR von SP+0x0100 lesen
		else if (c == 49) {
				CHK_RDY
				SR = Read(SP+0x0100)|32;
		}
		//R // TMPByte von PC-Adresse lesen // AC + TMPByte + Carry // PC+1
		else if (c == 50) {
				CHK_RDY
				tmp1 = Read(PC);

				if (IF_DECIMAL())
				{
						tmp = (AC & 0xF) + (tmp1 & 0xF) + (SR & 0x1);
						if (tmp > 0x9) tmp += 0x6;
						if (tmp <= 0x0F) tmp = (tmp & 0xF) + (AC & 0xF0) + (tmp1 & 0xF0);
						else tmp = (tmp & 0xF) + (AC & 0xF0) + (tmp1 & 0xF0) + 0x10;
						SET_ZERO(((AC + tmp1 + (SR & 0x1)) & 0xFF));
						SET_SIGN(tmp & 0x80);
						SET_OVERFLOW(((AC ^ tmp) & 0x80) && !((AC ^ tmp1) & 0x80));
						if ((tmp & 0x1F0) > 0x90) tmp += 0x60;
						SET_CARRY((tmp & 0xFF0) > 0xF0);
				}
				else
				{
						tmp = tmp1 + AC + (SR & 0x01);
						SET_SR_NZ(tmp & 0xff);
						SET_OVERFLOW(!((AC ^ tmp1) & 0x80) && ((AC ^ tmp) & 0x80));
						SET_CARRY(tmp > 0xFF);
				}
				AC = (unsigned char)tmp;
				PC++;
		}

		//R // TMPByte von Adresse lesen // AC + TMPByte + Carry
		else if (c == 51) {
				CHK_RDY
				tmp1 = Read(Adresse);

				if (IF_DECIMAL())
				{
						tmp = (AC & 0xF) + (tmp1 & 0xF) + (SR & 0x1);
						if (tmp > 0x9) tmp += 0x6;
						if (tmp <= 0x0F) tmp = (tmp & 0xF) + (AC & 0xF0) + (tmp1 & 0xF0);
						else tmp = (tmp & 0xF) + (AC & 0xF0) + (tmp1 & 0xF0) + 0x10;
						SET_ZERO(((AC + tmp1 + (SR & 0x1)) & 0xFF));
						SET_SIGN(tmp & 0x80);
						SET_OVERFLOW(((AC ^ tmp) & 0x80) && !((AC ^ tmp1) & 0x80));
						if ((tmp & 0x1F0) > 0x90) tmp += 0x60;
						SET_CARRY((tmp & 0xFF0) > 0xF0);
				}
				else
				{
						tmp = tmp1 + AC + (SR & 0x01);
						SET_SR_NZ(tmp & 0xff);
						SET_OVERFLOW(!((AC ^ tmp1) & 0x80) && ((AC ^ tmp) & 0x80));
						SET_CARRY(tmp > 0xFF);
				}
				AC = (unsigned char)tmp;
		}

		//R // TMPByte von Adresse lesen // Adresse Lo + XR
		else if (c == 52) {
				CHK_RDY
				TMPByte = Read(Adresse);
				Adresse += XR;
				Adresse &= 0xFF;
		}
		//R // Adresse Hi von PC-Adresse holen // Adresse+XR  // PC+1 //
		else if (c == 53) {
				CHK_RDY
				SetAdresseHi(Read(PC));
				Adresse += XR;
				idxReg = XR;
				PC++;
		}
		//R // Adresse Hi von PC-Adresse holen // Adresse+YR  // PC+1 //
		else if (c == 54) {
				CHK_RDY
				SetAdresseHi(Read(PC));
				Adresse += YR;
				idxReg = YR;
				PC++;
		}
		//R // TMPByte von Adresse lesen // AC + TMPByte + Carry // if(idxReg<Adresse Lo) MCT++
		else if (c == 55) {
				CHK_RDY
				if((Adresse&0xFF)>=(idxReg))
				{
						tmp1 = Read(Adresse);
						if (IF_DECIMAL())
						{
								tmp = (AC & 0xF) + (tmp1 & 0xF) + (SR & 0x1);
								if (tmp > 0x9) tmp += 0x6;
								if (tmp <= 0x0F) tmp = (tmp & 0xF) + (AC & 0xF0) + (tmp1 & 0xF0);
								else tmp = (tmp & 0xF) + (AC & 0xF0) + (tmp1 & 0xF0) + 0x10;
								SET_ZERO(((AC + tmp1 + (SR & 0x1)) & 0xFF));
								SET_SIGN(tmp & 0x80);
								SET_OVERFLOW(((AC ^ tmp) & 0x80) && !((AC ^ tmp1) & 0x80));
								if ((tmp & 0x1F0) > 0x90) tmp += 0x60;
								SET_CARRY((tmp & 0xFF0) > 0xF0);
						}
						else
						{
								tmp = tmp1 + AC + (SR & 0x01);
								SET_SR_NZ(tmp & 0xff);
								SET_OVERFLOW(!((AC ^ tmp1) & 0x80) && ((AC ^ tmp) & 0x80));
								SET_CARRY(tmp > 0xFF);
						}
						AC = (unsigned char)tmp;
						MCT++;
				}
		}
		//R // TMPByte von PC-Adresse lesen // AC - TMPByte - Carry // PC+1
		else if (c == 56) {
				CHK_RDY
				src=Read(PC);
				tmp2 = AC - src - ((SR & 0x01) ? 0 : 1);
				if (IF_DECIMAL())
				{
						tmp = (AC & 0xF) - (src & 0xF) - ((SR & 0x1) ? 0 : 1);
						if (tmp & 0x10) tmp = ((tmp - 6) & 0xF)| ((AC & 0xF0) - (src & 0xF0) - 0x10);
						else tmp = (tmp & 0xF) | ((AC & 0xF0) - (src & 0xF0));
						if (tmp & 0x100) tmp -= 0x60;
						SET_CARRY(tmp2 < 0x100);
						SET_SR_NZ(tmp2 & 0xFF);
						SET_OVERFLOW(((AC ^ tmp2) & 0x80) && ((AC ^ src) & 0x80));
						AC = (unsigned char) tmp;
				}
				else
				{
						SET_SR_NZ(tmp2 & 0xff);
						SET_CARRY(tmp2 < 0x100);
						SET_OVERFLOW(((AC ^ tmp2) & 0x80)
														 && ((AC ^ src) & 0x80));
						AC = (unsigned char) tmp2;
				}
				PC++;
		}
		//R // TMPByte von Adresse lesen // AC - TMPByte - Carry
		else if (c == 57) {
				CHK_RDY
				src=Read(Adresse);
				tmp2 = AC - src - ((SR & 0x01) ? 0 : 1);
				if (IF_DECIMAL())
				{
						tmp = (AC & 0xF) - (src & 0xF) - ((SR & 0x1) ? 0 : 1);
						if (tmp & 0x10) tmp = ((tmp - 6) & 0xF)| ((AC & 0xF0) - (src & 0xF0) - 0x10);
						else tmp = (tmp & 0xF) | ((AC & 0xF0) - (src & 0xF0));
						if (tmp & 0x100) tmp -= 0x60;
						SET_CARRY(tmp2 < 0x100);
						SET_SR_NZ(tmp2 & 0xFF);
						SET_OVERFLOW(((AC ^ tmp2) & 0x80) && ((AC ^ src) & 0x80));
						AC = (unsigned char) tmp;
				}
				else
				{
						SET_SR_NZ(tmp2 & 0xff);
						SET_CARRY(tmp2 < 0x100);
						SET_OVERFLOW(((AC ^ tmp2) & 0x80)
														 && ((AC ^ src) & 0x80));
						AC = (unsigned char) tmp2;
				}
		}
		//R // TMPByte von Adresse lesen // AC - TMPByte - Carry // if(idxReg<Adresse Lo) MCT++
		else if (c == 58) {
				CHK_RDY
				if((Adresse&0xFF)>=(idxReg))
				{
						src=Read(Adresse);
						tmp2 = AC - src - ((SR & 0x01) ? 0 : 1);
						if (IF_DECIMAL())
						{
								tmp = (AC & 0xF) - (src & 0xF) - ((SR & 0x1) ? 0 : 1);
								if (tmp & 0x10) tmp = ((tmp - 6) & 0xF)| ((AC & 0xF0) - (src & 0xF0) - 0x10);
								else tmp = (tmp & 0xF) | ((AC & 0xF0) - (src & 0xF0));
								if (tmp & 0x100) tmp -= 0x60;
								SET_CARRY(tmp2 < 0x100);
								SET_SR_NZ(tmp2 & 0xFF);
								SET_OVERFLOW(((AC ^ tmp2) & 0x80) && ((AC ^ src) & 0x80));
								AC = (unsigned char) tmp;
						}
						else
						{
								SET_SR_NZ(tmp2 & 0xff);
								SET_CARRY(tmp2 < 0x100);
								SET_OVERFLOW(((AC ^ tmp2) & 0x80)
																 && ((AC ^ src) & 0x80));
								AC = (unsigned char) tmp2;
						}
						MCT++;
				}
		}
		//R // TMPByte von SP+0x0100 holen
		else if (c == 59) {
				CHK_RDY
				TMPByte=Read(SP + 0x0100);
		}
		//W // PC-Adresse Hi nach SP+0x0100 schreiben // SP-1
		else if (c == 60) {
				Write(SP+0x0100,GetPCHi);
				SP--;
		}
		//W // PC-Adresse Lo nach SP+0x0100 schreiben // SP-1
		else if (c == 61) {
				Write(SP+0x0100,GetPCLo);
				SP--;
		}
		//R // TMPByte von SP+0x0100 holen // SP+1
		else if (c == 62) {
				CHK_RDY
				TMPByte = Read(SP+0x0100);
				SP++;
		}
		//R // PC-Adresse Lo von SP+0x0100 holen // SP+1
		else if (c == 63) {
				CHK_RDY
				SetPCLo(Read(SP+0x0100));
				SP++;
		}
		//R // PC-Adresse Hi von SP+0x0100 holen
		else if (c == 64) {
				CHK_RDY
				SetPCHi(Read(SP+0x0100));
		}
		//R // TMPByte von PC-Adresse laden // PC+1
		else if (c == 65) {
				CHK_RDY
				TMPByte = Read(PC);
				PC++;
		}
		//R // TMPByte von PC-Adresse lesen // AC and TMPByte // Set SR(NZ) // PC+1
		else if (c == 66) {
				CHK_RDY
				TMPByte=Read(PC);
				AC &= TMPByte;
				SET_SR_NZ(AC);
				PC++;
		}
		//R // TMPByte von Adresse lesen // AC and TMPByte // Set SR(NZ)
		else if (c == 67) {
				CHK_RDY
				TMPByte=Read(Adresse);
				AC &= TMPByte;
				SET_SR_NZ(AC);
		}
		//R // TMPByte von Adresse lesen // AC and TMPByte // Set SR(NZ) // if(idxReg<Adresse Lo) MCT++
		else if (c == 68) {
				CHK_RDY
				if((Adresse&0xFF)>=(idxReg))
				{
						TMPByte=Read(Adresse);
						AC &= TMPByte;
						SET_SR_NZ(AC);
						MCT++;
				}
		}
		//R // TMPByte von Adresse lesen // CarrayFalg=0
		else if (c == 69) {
				CHK_RDY
				TMPByte = Read(PC);
				SR &= 0xFE;
		}
		//R // TMPByte von Adresse lesen // DezimalFalg=0
		else if (c == 70) {
				CHK_RDY
				TMPByte = Read(PC);
				SR &= 0xF7;
		}
		//R // TMPByte von Adresse lesen // InterruptFalg=0
		else if (c == 71) {
				CHK_RDY
				TMPByte = Read(PC);
				irq_delay_sr_value = SR;
				SR &= 0xFB;
				irq_delay = true;
		}
		//R // TMPByte von Adresse lesen // OverflowFalg=0
		else if (c == 72) {
				CHK_RDY
				TMPByte = Read(PC);
				SR &= 0xBF;
		}
		//R // TMPByte von Adresse lesen // CarrayFalg=1
		else if (c == 73) {
				CHK_RDY
				TMPByte = Read(PC);
				SR |= 0x01;
		}
		//R // TMPByte von Adresse lesen // DezimalFalg=1
		else if (c == 74) {
				CHK_RDY
				TMPByte = Read(PC);
				SR |= 0x08;
		}
		//R // TMPByte von Adresse lesen // InterruptFalg=1
		else if (c == 75) {
				CHK_RDY
				TMPByte = Read(PC);
				irq_delay_sr_value = SR;
				SR |= 0x04;
				irq_delay = true;
		}
		//R // TMPByte von Adresse lesen // BIT Operation
		else if (c == 76) {
				CHK_RDY
				TMPByte = Read(Adresse);
				SET_ZERO(AC & TMPByte);
				SR = (TMPByte & 0xC0) | (SR & 0x3F);
		}
		//W // AC nach Adresse schreiben
		else if (c == 77) {
				Write(Adresse,AC);
		}
		//W // XR nach Adresse schreiben
		else if (c == 78) {
				Write(Adresse,XR);
		}
		//W // YR nach Adresse schreiben
		else if (c == 79) {
				Write(Adresse,YR);
		}
		//R // AC von PC-Adresse lesen // Set SR(NZ) // PC+1
		else if (c == 80) {
				CHK_RDY
				AC = Read(PC);
				SET_SR_NZ(AC);
				PC++;
		}
		//R // AC von Adresse lesen // Set SR(NZ)
		else if (c == 81) {
				CHK_RDY
				AC = Read(Adresse);
				SET_SR_NZ(AC);
		}
		//R // AC von Adresse lesen // Set SR(NZ) // if(idxReg<Adresse Lo) MCT++
		else if (c == 82) {
				CHK_RDY
				if((Adresse&0xFF)>=(idxReg))
				{
						TMPByte=Read(Adresse);
						AC = TMPByte;
						SET_SR_NZ(AC);
						MCT++;
				}
		}
		//R // XR von PC-Adresse lesen // Set SR(NZ) // PC+1
		else if (c == 83) {
				CHK_RDY
				XR = Read(PC);
				SET_SR_NZ(XR);
				PC++;
		}
		//R // XR von Adresse lesen // Set SR(NZ)
		else if (c == 84) {
				CHK_RDY
				XR = Read(Adresse);
				SET_SR_NZ(XR);
		}
		//R // XR von Adresse lesen // Set SR(NZ) // if(idxReg<Adresse Lo) MCT++
		else if (c == 85) {
				CHK_RDY
				if((Adresse&0xFF)>=(idxReg))
				{
						TMPByte=Read(Adresse);
						XR = TMPByte;
						SET_SR_NZ(XR);
						MCT++;
				}
		}
		//R // YR von PC-Adresse lesen // Set SR(NZ) // PC+1
		else if (c == 86) {
				CHK_RDY
				YR = Read(PC);
				SET_SR_NZ(YR);
				PC++;
		}
		//R // YR von Adresse lesen // Set SR(NZ)
		else if (c == 87) {
				CHK_RDY
				YR = Read(Adresse);
				SET_SR_NZ(YR);
		}
		//R // YR von Adresse lesen // Set SR(NZ) // if(idxReg<Adresse Lo) MCT++
		else if (c == 88) {
				CHK_RDY
				if((Adresse&0xFF)>=(idxReg))
				{
						TMPByte=Read(Adresse);
						YR = TMPByte;
						SET_SR_NZ(YR);
						MCT++;
				}
		}
		//R // TMPByte von Adresse lesen // XR+1 // Set SR(NZ)
		else if (c == 89) {
				CHK_RDY
				TMPByte = Read(PC);
				XR ++;
				SET_SR_NZ(XR);
		}
		//R // TMPByte von Adresse lesen // YR+1 // Set SR(NZ)
		else if (c == 90) {
				CHK_RDY
				TMPByte = Read(PC);
				YR ++;
				SET_SR_NZ(YR);
		}
		//R // TMPByte von Adresse lesen // XR-1 // Set SR(NZ)
		else if (c == 91) {
				CHK_RDY
				TMPByte = Read(PC);
				XR --;
				SET_SR_NZ(XR);
		}
		//R // TMPByte von Adresse lesen // YR-1 // Set SR(NZ)
		else if (c == 92) {
				CHK_RDY
				TMPByte = Read(PC);
				YR --;
				SET_SR_NZ(YR);
		}
		//R // Illegale Opcode //
		else if (c == 93) {
				//// Wird nie angesprungen !!! ////
		}
		//R // PC LO von Adresse lesen // AdresseLo+1
		else if (c == 94) {
				CHK_RDY
				SetPCLo(Read(Adresse));
				Adresse = (Adresse&0xFF00)|((Adresse+1)&0x00FF);
		}
		//R // PC HI von Adresse lesen
		else if (c == 95) {
				CHK_RDY
				SetPCHi(Read(Adresse));
		}
		//R // PC LO von $FFFC lesen
		else if (c == 96) {
				CHK_RDY
				SetPCLo(Read(0xFFFC));
		}
		//R // PC HI von $FFFD lesen
		else if (c == 97) {
				CHK_RDY
				SetPCHi(Read(0xFFFD));
		}
		//R // TMPByte von PC-Adresse lesen // AC - TMPByte (AC wird nicht verÃ€ndert) // Set SR(NZC) // PC+1
		else if (c == 98) {
				CHK_RDY
				TMPByte=Read(PC);
				tmp = AC - TMPByte;
				SET_CARRY(tmp < 0x100);
				SET_SIGN(tmp);
				SET_ZERO(tmp &= 0xFF);
				PC++;
		}
		//R // TMPByte von Adresse lesen // AC - TMPByte (AC wird nicht verÃ€ndert) // Set SR(NZC)
		else if (c == 99) {
				CHK_RDY
				TMPByte=Read(Adresse);
				tmp = AC - TMPByte;
				SET_CARRY(tmp < 0x100);
				SET_SIGN(tmp);
				SET_ZERO(tmp &= 0xFF);
		}
		//R // TMPByte von Adresse lesen // AC - TMPByte (AC wird nicht verÃ€ndert) // if(idxReg<Adresse Lo) MCT++
		else if (c == 100) {
				CHK_RDY
				if((Adresse&0xFF)>=(idxReg))
				{
						TMPByte=Read(Adresse);
						tmp = AC - TMPByte;
						SET_CARRY(tmp < 0x100);
						SET_SIGN(tmp);
						SET_ZERO(tmp &= 0xFF);
						MCT++;
				}
		}
		//R // TMPByte von PC-Adresse lesen // XR - TMPByte (XR wird nicht verÃ€ndert) // Set SR(NZC) // PC+1
		else if (c == 101) {
				CHK_RDY
				TMPByte=Read(PC);
				tmp = XR - TMPByte;
				SET_CARRY(tmp < 0x100);
				SET_SIGN(tmp);
				SET_ZERO(tmp &= 0xFF);
				PC++;
		}
		//R // TMPByte von Adresse lesen // XR - TMPByte (XR wird nicht verÃ€ndert) // Set SR(NZC)
		else if (c == 102) {
				CHK_RDY
				TMPByte=Read(Adresse);
				tmp = XR - TMPByte;
				SET_CARRY(tmp < 0x100);
				SET_SIGN(tmp);
				SET_ZERO(tmp &= 0xFF);
		}
		//R // TMPByte von PC-Adresse lesen // YR - TMPByte (XR wird nicht verÃ€ndert) // Set SR(NZC) // PC+1
		else if (c == 103) {
				CHK_RDY
				TMPByte=Read(PC);
				tmp = YR - TMPByte;
				SET_CARRY(tmp < 0x100);
				SET_SIGN(tmp);
				SET_ZERO(tmp &= 0xFF);
				PC++;
		}
		//R // TMPByte von Adresse lesen // YR - TMPByte (XR wird nicht verÃ€ndert) // Set SR(NZC)
		else if (c == 104) {
				CHK_RDY
				TMPByte=Read(Adresse);
				tmp = YR - TMPByte;
				SET_CARRY(tmp < 0x100);
				SET_SIGN(tmp);
				SET_ZERO(tmp &= 0xFF);
		}
		//R // TMPByte von PC-Adresse lesen // AC XOR TMPByte // Set SR(NZC) // PC+1
		else if (c == 105) {
				CHK_RDY
				TMPByte=Read(PC);
				AC^=TMPByte;
				SET_SR_NZ(AC);
				PC++;
		}
		//R // TMPByte von Adresse lesen // AC XOR TMPByte // Set SR(NZC)
		else if (c == 106) {
				CHK_RDY
				TMPByte=Read(Adresse);
				AC^=TMPByte;
				SET_SR_NZ(AC);
		}
		//R // TMPByte von Adresse lesen // AC XOR TMPByte // if(idxReg<Adresse Lo) MCT++
		else if (c == 107) {
				CHK_RDY
				if((Adresse&0xFF)>=(idxReg))
				{
						TMPByte=Read(Adresse);
						AC^=TMPByte;
						SET_SR_NZ(AC);
						MCT++;
				}
		}
		//R // TMPByte von PC-Adresse holen // AC>>1 // Set SR(NZC)
		else if (c == 108) {
				CHK_RDY
				TMPByte = Read(PC);
				SET_CARRY(AC&0x01);
				AC >>= 1;
				SET_SIGN(0);
				SET_ZERO(AC);
		}
		//W // TMPByte nach Adresse schreiben // TMPByte>>1 // Set SR(NZC)
		else if (c == 109) {
				Write(Adresse,TMPByte);
				SET_CARRY(TMPByte&0x01);
				TMPByte >>= 1;
				SET_SIGN(0);
				SET_ZERO(TMPByte);
		}
		//R // TMPByte von PC-Adresse holen // C<-AC<<1<-C // Set SR(NZC)
		else if (c == 110) {
				CHK_RDY
				TMPByte = Read(PC);
				tmp = AC;
				tmp <<= 1;
				if (IF_CARRY()) tmp |= 0x1;
				SET_CARRY(tmp > 0xff);
				tmp &= 0xff;
				SET_SIGN(tmp);
				SET_ZERO(tmp);
				AC = tmp;
		}
		//W // TMPByte nach Adresse schreiben // C<-TMPByte<<1<-C // Set SR(NZC)
		else if (c == 111) {
				Write(Adresse,TMPByte);
				tmp = TMPByte;
				tmp <<= 1;
				if (IF_CARRY()) tmp |= 0x1;
				SET_CARRY(tmp > 0xff);
				tmp &= 0xff;
				SET_SIGN(tmp);
				SET_ZERO(tmp);
				TMPByte = tmp;
		}
		//R // TMPByte von PC-Adresse holen // C->AC>>1->C // Set SR(NZC)
		else if (c == 112) {
				CHK_RDY
				TMPByte = Read(PC);
				carry_tmp=IF_CARRY();
				SET_CARRY(AC & 0x01);
				AC >>= 1;
				if(carry_tmp) AC |= 0x80;
				SET_SIGN(AC);
				SET_ZERO(AC);
		}
		//W // TMPByte nach Adresse schreiben // C->TMPByte>>1->C // Set SR(NZC)
		else if (c == 113) {
				Write(Adresse,TMPByte);
				carry_tmp=IF_CARRY();
				SET_CARRY(TMPByte & 0x01);
				TMPByte >>= 1;
				if(carry_tmp) TMPByte |= 0x80;
				SET_SIGN(TMPByte);
				SET_ZERO(TMPByte);
		}
		//W // TMPByte nach Adresse schreiben // TMPByte+1 // Set SR(NZ)
		else if (c == 114) {
				Write(Adresse,TMPByte);
				TMPByte++;
				SET_SR_NZ(TMPByte);
		}
		//W // TMPByte nach Adresse schreiben // TMPByte-1 // Set SR(NZ)
		else if (c == 115) {
				Write(Adresse,TMPByte);
				TMPByte--;
				SET_SR_NZ(TMPByte);
		}
		//W // SR nach 0x100+SP schreiben // SP-- // IFlag setzen // BFlag lÃ¶schen
		else if (c == 116) {
				SR&=239;
				Write(0x0100+SP,SR);
				SP--;
				SR|=4;
		}
		//R // PC Lo von 0xFFFA holen
		else if (c == 117) {
				CHK_RDY
				SetPCLo(Read(0xFFFA));
		}
		//R // PC Hi von 0xFFFB holen
		else if (c == 118) {
				CHK_RDY
				SetPCHi(Read(0xFFFB));
		}
		//R // TMPByte von Adresse holen // Fix Adresse Hi MCT+1
		else if (c == 119) {
				CHK_RDY
				if((Adresse&0xFF)>=(idxReg))
				{
						TMPByte = Read(Adresse);
						MCT++;
				}
		}
		//W // TMPByte nach Adresse schreiben // Illegal [SLO]
		else if (c == 120) {
				Write(Adresse,TMPByte);
				SET_CARRY(TMPByte&0x80);	// ASL MEMORY
				TMPByte <<= 1;
				AC |= TMPByte;				// ORA
				SET_SR_NZ(AC);
		}
		//W // TMPByte nach Adresse schreiben // Illegal [RLA]
		else if (c == 121) {
				Write(Adresse,TMPByte);
				tmp = TMPByte;
				tmp <<= 1;					// ROL
				if (IF_CARRY()) tmp |= 0x1;
				SET_CARRY(tmp > 0xFF);
				tmp &= 0xFF;
				TMPByte = tmp;
				AC &= TMPByte;				// AND
				SET_SR_NZ(AC);
		}
		//W // TMPByte nach Adresse schreiben // Illegal [SRE]
		else if (c == 122) {
				Write(Adresse,TMPByte);
				SET_CARRY(TMPByte & 0x01);	// LSR
				TMPByte >>= 1;
				AC ^= TMPByte;				// EOR
				SET_SR_NZ(AC);
		}
		//W // TMPByte nach Adresse schreiben // Illegal [RRA]
		else if (c == 123) {
				Write(Adresse,TMPByte);

				carry_tmp = IF_CARRY();		// ROR
				SET_CARRY(TMPByte & 0x01);
				TMPByte >>= 1;
				if(carry_tmp) TMPByte |= 0x80;

				tmp1 = TMPByte;				// ADC
				if (IF_DECIMAL())
				{
						tmp = (AC & 0xF) + (tmp1 & 0xF) + (SR & 0x1);
						if (tmp > 0x9) tmp += 0x6;
						if (tmp <= 0x0F) tmp = (tmp & 0xF) + (AC & 0xF0) + (tmp1 & 0xF0);
						else tmp = (tmp & 0xF) + (AC & 0xF0) + (tmp1 & 0xF0) + 0x10;
						SET_ZERO(((AC + tmp1 + (SR & 0x1)) & 0xFF));
						SET_SIGN(tmp & 0x80);
						SET_OVERFLOW(((AC ^ tmp) & 0x80) && !((AC ^ tmp1) & 0x80));
						if ((tmp & 0x1F0) > 0x90) tmp += 0x60;
						SET_CARRY((tmp & 0xFF0) > 0xF0);
				}
				else
				{
						tmp = tmp1 + AC + (SR & 0x01);
						SET_SR_NZ(tmp & 0xff);
						SET_OVERFLOW(!((AC ^ tmp1) & 0x80) && ((AC ^ tmp) & 0x80));
						SET_CARRY(tmp > 0xFF);
				}
				AC = (unsigned char)tmp;
		}
		//W // TMPByte nach Adresse schreiben // Illegal [DCP]
		else if (c == 124) {
				Write(Adresse,TMPByte);
				TMPByte--;					//DEC

				tmp = AC - TMPByte;			//CMP
				SET_CARRY(tmp < 0x100);
				SET_SIGN(tmp);
				SET_ZERO(tmp &= 0xFF);
		}
		//W // TMPByte nach Adresse schreiben // Illegal [ISB]
		else if (c == 125) {
				Write(Adresse,TMPByte);
				TMPByte++;					//INC

				src=TMPByte;				//SBC
				tmp2 = AC - src - ((SR & 0x01) ? 0 : 1);
				if (IF_DECIMAL())
				{
						tmp = (AC & 0xF) - (src & 0xF) - ((SR & 0x1) ? 0 : 1);
						if (tmp & 0x10) tmp = ((tmp - 6) & 0xF)| ((AC & 0xF0) - (src & 0xF0) - 0x10);
						else tmp = (tmp & 0xF) | ((AC & 0xF0) - (src & 0xF0));
						if (tmp & 0x100) tmp -= 0x60;
						SET_CARRY(tmp2 < 0x100);
						SET_SR_NZ(tmp2 & 0xFF);
						SET_OVERFLOW(((AC ^ tmp2) & 0x80) && ((AC ^ src) & 0x80));
						AC = (unsigned char) tmp;
				}
				else
				{
						SET_SR_NZ(tmp2 & 0xff);
						SET_CARRY(tmp2 < 0x100);
						SET_OVERFLOW(((AC ^ tmp2) & 0x80)
														 && ((AC ^ src) & 0x80));
						AC = (unsigned char) tmp2;
				}
		}
		//R // AC von Adresse lesen // AC -> XR // Set SR(NZ) // Illegal [LAX]
		else if (c == 126) {
				CHK_RDY
				AC = Read(Adresse);
				XR = AC;
				SET_SR_NZ(AC);
		}
		//R // AC von Adresse lesen // AC -> XR // Set SR(NZ) // if(idxReg<Adresse Lo) MCT++ // Illegal [LAX]
		else if (c == 127) {
				CHK_RDY
				if((Adresse&0xFF)>=(idxReg))
				{
						TMPByte=Read(Adresse);
						AC = TMPByte;
						XR = AC;
						SET_SR_NZ(AC);
						MCT++;
				}
		}
		//W // TMPByte = AC & XR // TMPByte nach Adresse schreiben // Set SR(NZ) // Illegal [SAX]
		else if (c == 128) {
				TMPByte = AC & XR;
				Write(Adresse,TMPByte);
		}
		//R // Illegal [ASR]
		else if (c == 129) {
				CHK_RDY
				AC &= Read(PC);				// AND

				SET_CARRY(AC & 0x01);		// LSR
				AC >>= 1;
				SET_SIGN(0);
				SET_ZERO(AC);
				PC++;
		}
		//R // Illegal [ARR]
		else if (c == 130) {
				CHK_RDY
				tmp2 = Read(PC) & AC;
				AC = ((SR & 0x01) ? (tmp2 >> 1) | 0x80 : tmp2 >> 1);
				if (!IF_DECIMAL())
				{
						SET_SR_NZ(AC);
						SET_CARRY(AC & 0x40);
						SET_OVERFLOW(!!((AC & 0x40) ^ ((AC & 0x20) << 1)));
				}
				else
				{
						SET_SIGN((SR & 0x01) ? 0x80 : 0);
						SET_ZERO(AC);
						SET_OVERFLOW(!!((tmp2 ^ AC) & 0x40));
						if ((tmp2 & 0x0F) + (tmp2 & 0x01) > 5) AC = (AC & 0xF0) | ((AC + 6) & 0x0F);
						SET_CARRY(((tmp2 + (tmp2 & 0x10)) & 0x1F0)>0x50);
						if(IF_CARRY()) AC += 0x60;
				}
				PC++;
		}
		//R // Illegal [ANE]
		else if (c == 131) {
				CHK_RDY
				TMPByte = Read(PC);
				AC = (AC | 0xEE) & XR & TMPByte;
				SET_SR_NZ(AC);
				PC++;
		}
		//R // Illegal [LXA]
		else if (c == 132) {
				CHK_RDY
				TMPByte = Read(PC);
				AC = XR = ((AC | 0xEE) & TMPByte);
				SET_SR_NZ(AC);
				PC++;
		}
		//R // Illegal [SBX]
		else if (c == 133) {
				CHK_RDY
				tmp = Read(PC);
				tmp = (AC & XR) - tmp;
				SET_CARRY(tmp < 0x100);
				XR = tmp & 0xFF;
				SET_SR_NZ(XR);
				PC++;
		}
		//W // Illegal [SHY]
		else if (c == 134) {
				if((Adresse & 0xFF) < XR)
				{
					// page boundary crossing
					axa_byte = (Adresse >> 8) & YR;
					Adresse = (Adresse & 0xFF) | axa_byte << 8;
				}
				else
					axa_byte = YR & ((Adresse >> 8) + 1);
				if(shxy_dma)
				{
					axa_byte = YR;
					shxy_dma = false;
				}
				Write(Adresse, axa_byte);
		}
		//W // Illegal [SHX]
		else if (c == 135) {
			if((Adresse & 0xFF) < YR)
			{
				// page boundary crossing
				axa_byte = (Adresse >> 8) & XR;
				Adresse = (Adresse & 0xFF) | axa_byte << 8;
			}
			else
				axa_byte = XR & ((Adresse >> 8) + 1);
			if(shxy_dma)
			{
				axa_byte = XR;
				shxy_dma = false;
			}
			Write(Adresse, axa_byte);
	}
		//W // Illegal [SHA]
		else if (c == 136) {
				Write(Adresse, AC & XR & ((Adresse >> 8) + 1));
		}
		//W // Illegal [SHS]
		else if (c == 137) {
				SP = AC & XR;
				Write(Adresse, AC & XR & ((Adresse >> 8) + 1));
		}
		//R // Illegal [ANC]
		else if (c == 138) {
				CHK_RDY
				AC &= Read(PC);
				SET_SR_NZ(AC);
				SET_CARRY(SR&0x80);
				PC++;
		}
		//R // Illegal [LAE]
		else if (c == 139) {
				CHK_RDY
				TMPByte = Read(Adresse);
				AC = XR = SP = SP & (TMPByte);
				SET_SR_NZ(AC);
		}
		//R // Illegal [LAE]
		else if (c == 140) {
				CHK_RDY
				if((Adresse&0xFF)>=(idxReg))
				{
						TMPByte=Read(Adresse);
						AC = XR = SP = SP & (TMPByte);
						SET_SR_NZ(AC);
						MCT++;
				}
		}
L1:
		MCT++;

		if(*MCT == 0)
		{
			AktOpcodePC = PC;
			/**
			if(Breakpoints[PC] & 1)
			{
					*BreakStatus |=1;
					BreakWerte[0] = PC;
			}
			if(Breakpoints[AC] & 2)
			{
					*BreakStatus |=2;
					BreakWerte[1] = AC;
			}
			if(Breakpoints[XR] & 4)
			{
					*BreakStatus |=4;
					BreakWerte[2] = XR;
			}
			if(Breakpoints[YR] & 8)
			{
					*BreakStatus |=8;
					BreakWerte[3] = YR;
			}
			*/
			if(ResetReadyAdr == PC)
			{
				*ResetReady = true;
			}
			return true;
		}
		else return false;
	}
	return false;
}

void MOS6510::Phi1()
{
	if(irq_is_low_pegel)
	{
		irq_is_active = true;
	}
	else irq_is_active = false;

	if(nmi_fall_edge)
		nmi_is_active = true;
}

MOS6510_PORT::MOS6510_PORT(void)
{
	Reset();
}

MOS6510_PORT::~MOS6510_PORT(void)
{
}

void MOS6510_PORT::Reset(void)
{
	DATA = 0x3f;
	DATA_OUT = 0x3f;
	DATA_READ = 0x3f;
	DIR = 0;
	DIR_READ = 0;
	DATA_SET_BIT6 = 0;
	DATA_SET_BIT7 = 0;
	DATA_FALLOFF_BIT6 = 0;
	DATA_FALLOFF_BIT7 = 0;

    TAPE_SENSE = false;
}

void MOS6510_PORT::ConfigChanged(int tape_sense, int caps_sense,unsigned char pullup)
{
	TAPE_SENSE = !!(tape_sense);
	/* Tape Motor Status.  */
	static unsigned char OLD_DATA_OUT = 0xFF;
	/* Tape Write Line Status */
	static unsigned char OLD_WRITE_BIT = 0xFF;

	DATA_OUT = (DATA_OUT & ~DIR)|(DATA & DIR);
	DATA_READ = (DATA | ~DIR) & (DATA_OUT | pullup);

	if ((pullup & 0x40) && !caps_sense) DATA_READ &= 0xBF;
	if (!(DIR & 0x20)) DATA_READ &= 0xDF;
	if (tape_sense && !(DIR & 0x10))DATA_READ &= 0xEF;
	if (((DIR & DATA) & 0x20) != OLD_DATA_OUT)
	{
		OLD_DATA_OUT = (DIR & DATA) & 0x20;
		DATASETTE_MOTOR = !OLD_DATA_OUT;
	}

	if (((~DIR | DATA) & 0x8) != OLD_WRITE_BIT)
	{
		OLD_WRITE_BIT = (~DIR | DATA) & 0x8;
		//datasette_toggle_write_bit((~pport.dir | pport.data) & 0x8);
	}
	DIR_READ = DIR;
}
/*
bool MOS6510_PORT::SaveFreez(FILE* File)
{
	fwrite(&DIR,1,sizeof(DIR),File);
	fwrite(&DATA,1,sizeof(DATA),File);
	fwrite(&DIR_READ,1,sizeof(DIR_READ),File);
	fwrite(&DATA_READ,1,sizeof(DATA_READ),File);
	fwrite(&DATA_OUT,1,sizeof(DATA_OUT),File);
	fwrite(&DATA_SET_CLK_BIT6,1,sizeof(DATA_SET_CLK_BIT6),File);
	fwrite(&DATA_SET_CLK_BIT7,1,sizeof(DATA_SET_CLK_BIT7),File);
	fwrite(&DATA_SET_BIT6,1,sizeof(DATA_SET_BIT6),File);
	fwrite(&DATA_SET_BIT7,1,sizeof(DATA_SET_BIT7),File);
	fwrite(&DATA_FALLOFF_BIT6,1,sizeof(DATA_FALLOFF_BIT6),File);
	fwrite(&DATA_FALLOFF_BIT7,1,sizeof(DATA_FALLOFF_BIT7),File);
	return true;
}

bool MOS6510_PORT::LoadFreez(FILE* File,unsigned short Version)
{
	switch(Version)
	{
	else if (c == 0x0100) {
	else if (c == 0x0101) {
		fread(&DIR,1,sizeof(DIR),File);
		fread(&DATA,1,sizeof(DATA),File);
		fread(&DIR_READ,1,sizeof(DIR_READ),File);
		fread(&DATA_READ,1,sizeof(DATA_READ),File);
		fread(&DATA_OUT,1,sizeof(DATA_OUT),File);
		fread(&DATA_SET_CLK_BIT6,1,sizeof(DATA_SET_CLK_BIT6),File);
		fread(&DATA_SET_CLK_BIT7,1,sizeof(DATA_SET_CLK_BIT7),File);
		fread(&DATA_SET_BIT6,1,sizeof(DATA_SET_BIT6),File);
		fread(&DATA_SET_BIT7,1,sizeof(DATA_SET_BIT7),File);
		fread(&DATA_FALLOFF_BIT6,1,sizeof(DATA_FALLOFF_BIT6),File);
		fread(&DATA_FALLOFF_BIT7,1,sizeof(DATA_FALLOFF_BIT7),File);
		break;
	}
	return true;
}
*/
