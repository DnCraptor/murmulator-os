#include "mos6510.h"
#include "micro_code_tbl_6510.h"

#define CHK_RDY	if(!*p->RDY && !p->CpuWait){p->CpuWait=true;p->MCT--;break;}
#define OLD_IRQHandling
#define GetPCLo ((unsigned char)p->PC)
#define GetPCHi ((unsigned char)(p->PC>>8))
#define SetPCLo(wert) p->PC=((p->PC&0xFF00)|wert)
#define SetPCHi(wert) p->PC=((p->PC&0x00FF)|(wert<<8))
#define SetAdresseLo(wert) p->Adresse = ((p->Adresse&0xFF00)|wert)
#define SetAdresseHi(wert) p->Adresse = ((p->Adresse&0x00FF)|(wert<<8))

static void MOS6510_MOS6510(MOS6510* p)
{
	p->MCT = ((unsigned char*)MicroCodeTable6510 + (0x100*MCTItemSize));
	p->AktOpcodePC = 0x0100;
	p->SR = 32;
}

static void MOS6510_Write(MOS6510* p, unsigned short adresse, unsigned char wert)
{
	if((p->EnableDebugCart == true) && (adresse == DEBUG_CART_ADRESS))
	{
		p->DebugCartValue = wert;
		p->WRITE_DEBUG_CART = true;
	}

	if(adresse == 0xFF00) p->WRITE_FF00 = true;
    func_write_t* fn = &p->WriteProcTbl[(adresse)>>8];
    if (fn->p)
	    fn->fn(fn->p, adresse, wert);
}

// Phi2
static bool MOS6510_OneZyklus(MOS6510* p)
{
	static bool RESET_OLD = false;
	static unsigned char   idxReg = 0;

	unsigned int    tmp;
	unsigned int    tmp1;
	unsigned short  tmp2;
	unsigned short  src;
	bool            carry_tmp;
	if(*p->RDY) {
        p->CpuWait = false;
    }

	if(!*p->RESET)
	{
		p->CpuWait = true;
	}

	if((*p->RESET == true) && (RESET_OLD == false))
	{
		p->CpuWait = false;
		p->Interrupts[CIA_NMI] = false;
		p->Interrupts[CRT_NMI] = false;
		p->Interrupts[RESTORE_NMI] = false;

		p->JAMFlag = false;
		p->SR = 0x04;
		p->MCT = ((unsigned char*)MicroCodeTable6510 + (0x100*MCTItemSize));
		p->AktOpcode = 0x100;
	}
	RESET_OLD = *p->RESET;

	// IRQ auf low Pegel prüfen
	if(p->irq_state > 0)
		p->irq_is_low_pegel = true;
	else
		p->irq_is_low_pegel = false;

	// NMI auf fallende Flanke überprüfen
	if(p->nmi_state == true && p->nmi_state_old == false)
		p->nmi_fall_edge = true;
	else
		p->nmi_fall_edge = false;
	p->nmi_state_old = p->nmi_state;

	if(!p->CpuWait)
	{
Disassemble(p, get_stdout(), p->PC, true);
		switch(*p->MCT)
		{
		//R // Feetch Opcode
		case 0:
			if(p->JAMFlag) return false;

			CHK_RDY

			if(p->nmi_is_active == true) // NMIStatePuffer[CYCLES] --> 2 CYCLES Sagt zwei Zyklen vorher muss der NMI schon angelegen haben also vor dem letzten Zyklus des vorigen Befehls
			{
				p->nmi_is_active = false;
				p->MCT = ((unsigned char*)MicroCodeTable6510 + (0x102*MCTItemSize));
				p->AktOpcode = 0x102;

				p->irq_delay = false;
				return false;
			}
			else
			{
				if((p->irq_is_active == true) && (p->irq_delay ? ((p->irq_delay_sr_value&4)==0) : ((p->SR&4)==0))) // IRQLinePuffer[CYCLES] --> 2 CYCLES Sagt zwei Zyklen vorher muss der IRQ schon anliegen also vor dem letzten Zyklus des vorigen Befehls
				{
					p->MCT = ((unsigned char*)MicroCodeTable6510 + (0x101*MCTItemSize));
					p->AktOpcode = 0x101;

					p->irq_delay = false;
					return false;
				}
			}

			p->irq_delay = false;

			p->MCT = ((unsigned char*)MicroCodeTable6510 + (MOS6510_Read(p, p->PC)*MCTItemSize));
            func_read_t* fn = &p->ReadProcTbl[(p->AktOpcodePC)>>8];
			p->AktOpcode = fn->p ? fn->fn(fn->p, p->AktOpcodePC) : 0;

			p->PC++;

			return false;
		break;

		//R // Lesen von PC-p->Adresse und verwerfen // PC++
		case 1:
			CHK_RDY
			MOS6510_Read(p, p->PC);
			p->PC++;
			break;
		//W // PC Hi -> Stack // p->SR|16 // SP--
		case 2:
			MOS6510_Write(p, 0x0100+(p->SP),GetPCHi);
			p->SR|=16;
			p->SP--;
			break;
		//W // PC Lo -> Stack // SP--
		case 3:
			MOS6510_Write(p, 0x0100+(p->SP),GetPCLo);
			p->SP--;
			break;
		//W // p->SR -> Stack // p->SR|4 // SP--
		case 4:
			MOS6510_Write(p, 0x0100+(p->SP),p->SR);
			p->SR|=4;
			p->SP--;
			break;
		//R // PC Lo von 0xFFFE holen
		case 5:
			CHK_RDY
			SetPCLo(MOS6510_Read(p, 0xFFFE));
			break;
		//R // PC Hi von 0xFFFF holen
		case 6:
			CHK_RDY
			SetPCHi(MOS6510_Read(p, 0xFFFF));
			break;
		//R // p->Pointer von PC-p->Adresse holen // PC++
		case 7:
			CHK_RDY
			p->Pointer = MOS6510_Read(p, p->PC);
			p->PC++;
			break;
		//R // Lesen von p->Pointer und verwerfen // p->Pointer+p->XR
		case 8:
			CHK_RDY
			MOS6510_Read(p, (unsigned short)p->Pointer);
			p->Pointer += p->XR;
			break;
		//R // p->Adresse Lo von p->Pointer-p->Adresse holen // p->Pointer++
		case 9:
				CHK_RDY
				SetAdresseLo(MOS6510_Read(p, (unsigned short)p->Pointer));
				p->Pointer++;
				break;
		//R // p->Adresse Hi von p->Pointer-p->Adresse holen //
		case 10:
				CHK_RDY
				SetAdresseHi(MOS6510_Read(p, (unsigned short)p->Pointer));
				break;
		//R // p->TMPByte von p->Adresse holen // p->AC or p->TMPByte // Set p->SR(NZ)
		case 11:
				CHK_RDY
				p->TMPByte = MOS6510_Read(p, p->Adresse);
				p->AC |= p->TMPByte;
				MOS6510_SET_SR_NZ(p, p->AC);
				break;
		//R // JAM
		case 12:
				CHK_RDY
				p->CpuWait=false;
				p->SR = 0x04;
				p->MCT = ((unsigned char*)MicroCodeTable6510 + (0x100*MCTItemSize));
				return 0;
		break;
		//R // p->TMPByte von p->Adresse holen
		case 13:
				// Prüfen ob DMA statt findet
				if(!p->shxy_dma)
				{
				if(!*p->RDY && !p->CpuWait)
					p->shxy_dma = true;
				else
					p->shxy_dma = false;
				}

				CHK_RDY
				p->TMPByte = MOS6510_Read(p, p->Adresse);
				break;
		//W // p->TMPByte nach p->Adresse schreiben // ASL MEMORY // ORA
		case 14:
			MOS6510_Write(p, p->Adresse,p->TMPByte);

			MOS6510_SET_CARRY(p, p->TMPByte&0x80);	// ASL MEMORY
			p->TMPByte <<= 1;
			p->TMPByte &= 0xFF;

			p->AC|=p->TMPByte;				// ORA
			MOS6510_SET_SR_NZ(p, p->AC);

			break;
		//W // p->TMPByte nach p->Adresse schreiben
		case 15:
				MOS6510_Write(p, p->Adresse,p->TMPByte);
				break;
		//R // p->Adresse Hi = 0 // p->Adresse Lo von PC-p->Adresse holen // PC++
		case 16:
				CHK_RDY
				p->Adresse = MOS6510_Read(p, p->PC);
				p->PC++;
				break;
		//R // p->TMPByte von p->Adresse lesen // p->Adresse Lo + p->YR
		case 17:
				CHK_RDY
				p->TMPByte = MOS6510_Read(p, p->Adresse);
				p->Adresse += p->YR;
				p->Adresse &= 0xFF;
				break;
		//W // p->TMPByte nach p->Adresse schreiben // p->TMPByte<<1 // Set p->SR(NZC)
		case 18:
				MOS6510_Write(p, p->Adresse,p->TMPByte);
				MOS6510_SET_CARRY(p, p->TMPByte&0x80);
				p->TMPByte <<= 1;
				p->TMPByte &= 0xFF;
				MOS6510_SET_SIGN(p, p->TMPByte);
				MOS6510_SET_ZERO(p, p->TMPByte);
				break;
		//R // p->TMPByte von PC-p->Adresse holen
		case 19:
				CHK_RDY
				p->TMPByte = MOS6510_Read(p, p->PC);
				break;
		//W // p->SR nach SP+0x0100 schreiben // SP-1
		case 20:
				MOS6510_Write(p, p->SP + 0x0100, p->SR | 16);
				p->SP--;
				break;
		//R // p->TMPByte von PC-p->Adresse holen // p->AC or p->TMPByte // PC+1
		case 21:
				CHK_RDY
				p->TMPByte = MOS6510_Read(p, p->PC);
				p->AC|=p->TMPByte;
				MOS6510_SET_SR_NZ(p, p->AC);
				p->PC++;
				break;
		//R // p->TMPByte von PC-p->Adresse holen // p->AC<<1 // Set p->SR(NZC)
		case 22:
				CHK_RDY
				p->TMPByte = MOS6510_Read(p, p->PC);
				MOS6510_SET_CARRY(p, p->AC&0x80);
				p->AC <<= 1;
				p->AC &= 0xFF;
				MOS6510_SET_SIGN(p, p->AC);
				MOS6510_SET_ZERO(p, p->AC);
				break;
		//R // p->TMPByte von PC-p->Adresse holen // p->AC and p->TMPByte // Set p->SR(NZC) // PC+1
		case 23:
				CHK_RDY
				p->TMPByte = MOS6510_Read(p, p->PC);
				p->AC&=p->TMPByte;
				MOS6510_SET_SR_NZ(p, p->AC);
				p->SR&=0xFE;
				p->SR|=p->AC>>7;
				p->PC++;
				break;
		//R // p->Adresse Lo von PC-p->Adresse holen // PC+1
		case 24:
				CHK_RDY
				SetAdresseLo(MOS6510_Read(p, p->PC));
				p->PC++;
				break;
		//R // p->Adresse Hi von PC-p->Adresse holen // PC+1
		case 25:
				CHK_RDY
				SetAdresseHi(MOS6510_Read(p, p->PC));
				p->PC++;
				break;
		//R // p->TMPByte von PC-p->Adresse holen // PC+1 // p->SR(N) auf FALSE prÃŒfen (BPL)
		case 26:
				CHK_RDY
				p->TMPByte = MOS6510_Read(p, p->PC);
				p->PC++;
				if((p->SR&0x80)!=0x00) p->MCT+=2;
				break;
		//R // p->TMPByte von PC-p->Adresse holen // PC+1 // p->SR(N) auf TRUE prÃŒfen (BMI)
		case 27:
				CHK_RDY
				p->TMPByte = MOS6510_Read(p, p->PC);
				p->PC++;
				if((p->SR&0x80)!=0x80) p->MCT+=2;
				break;
		//R // p->TMPByte von PC-p->Adresse holen // PC+1 // p->SR(V) auf FALSE prÃŒfen (BVC)
		case 28:
				CHK_RDY
				p->TMPByte = MOS6510_Read(p, p->PC);
				p->PC++;
				if((p->SR&0x40)!=0x00) p->MCT+=2;
				break;
		//R // p->TMPByte von PC-p->Adresse holen // PC+1 // p->SR(V) auf TRUE prÃŒfen (BVS)
		case 29:
				CHK_RDY
				p->TMPByte = MOS6510_Read(p, p->PC);
				p->PC++;
				if((p->SR&0x40)!=0x40) p->MCT+=2;
				break;
		//R // p->TMPByte von PC-p->Adresse holen // PC+1 // p->SR(C) auf FALSE prÃŒfen (BCC)
		case 30:
				CHK_RDY
				p->TMPByte = MOS6510_Read(p, p->PC);
				p->PC++;
				if((p->SR&0x01)!=0x00) p->MCT+=2;
				break;
		//R // p->TMPByte von PC-p->Adresse holen // PC+1 // p->SR(C) auf TRUE prÃŒfen (BCS)
		case 31:
				CHK_RDY
				p->TMPByte = MOS6510_Read(p, p->PC);
				p->PC++;
				if((p->SR&0x01)!=0x01) p->MCT+=2;
				break;
		//R // p->TMPByte von PC-p->Adresse holen // PC+1 // p->SR(Z) auf FALSE prÃŒfen (BNE)
		case 32:
				CHK_RDY
				p->TMPByte = MOS6510_Read(p, p->PC);
				p->PC++;
				if((p->SR&0x02)!=0x00) p->MCT+=2;
				break;
		//R // p->TMPByte von PC-p->Adresse holen // PC+1 // p->SR(Z) auf TRUE prÃŒfen (BEQ)
		case 33:
				CHK_RDY
				p->TMPByte = MOS6510_Read(p, p->PC);
				p->PC++;
				if((p->SR&0x02)!=0x02) p->MCT+=2;
				break;
		//R // Lesen von PC-p->Adresse und verwerfen // p->BranchAdresse=PC+p->TMPByte
		case 34:
				CHK_RDY
				MOS6510_Read(p, p->PC);
				p->BranchAdresse = p->PC + (signed char)(p->TMPByte);
				if ((p->PC ^ p->BranchAdresse) & 0xFF00)
				{
					p->PC = (p->PC & 0xFF00) | (p->BranchAdresse&0xFF);
				}
				else
				{
					p->PC = p->BranchAdresse;
					p->MCT+=1;
				}
				break;
		//R // FIX PC Hi p->Adresse (Im Branchbefehl)
		case 35:
				CHK_RDY
				MOS6510_Read(p, p->PC);
				p->PC = p->BranchAdresse;
				break;
		//R // p->Adresse Hi von p->Pointer-p->Adresse holen // p->Adresse+p->YR
		case 36:
				CHK_RDY
				SetAdresseHi(MOS6510_Read(p, (unsigned short)p->Pointer));
				p->Adresse += p->YR;
				idxReg = p->YR;
				break;
		//R // p->TMPByte von p->Adresse holen // Fix p->Adresse Hi p->MCT+1 // p->AC or p->TMPByte //
		case 37:
				CHK_RDY
				if((p->Adresse&0xFF) >= idxReg)
				{
						p->TMPByte = MOS6510_Read(p, p->Adresse);
						p->AC|=p->TMPByte;
						MOS6510_SET_SR_NZ(p, p->AC);
						p->MCT++;
				}
				break;
		//R // p->Adresse Hi von PC-p->Adresse holen // PC=p->Adresse
		case 38:
				CHK_RDY
				SetAdresseHi(MOS6510_Read(p, p->PC));
				p->PC = p->Adresse;
				break;
		//R // Lesen von PC-p->Adresse und verwerfen // p->XR=p->AC // Set p->SR(NZ)
		case 39:
				CHK_RDY
				MOS6510_Read(p, p->PC);
				p->XR = p->AC;
				MOS6510_SET_SR_NZ(p, p->XR);
				break;
		//R // Lesen von PC-p->Adresse und verwerfen // p->YR=p->AC // Set p->SR(NZ)
		case 40:
				CHK_RDY
				MOS6510_Read(p, p->PC);
				p->YR = p->AC;
				MOS6510_SET_SR_NZ(p, p->YR);
				break;
		//R // Lesen von PC-p->Adresse und verwerfen // p->XR=SP // Set p->SR(NZ)
		case 41:
				CHK_RDY
				MOS6510_Read(p, p->PC);
				p->XR = p->SP;
				MOS6510_SET_SR_NZ(p, p->XR);
				break;
		//R // Lesen von PC-p->Adresse und verwerfen // p->AC=p->XR // Set p->SR(NZ)
		case 42:
				CHK_RDY
				MOS6510_Read(p, p->PC);
				p->AC = p->XR;
				MOS6510_SET_SR_NZ(p, p->AC);
				break;
		//R // Lesen von PC-p->Adresse und verwerfen // SP=p->XR
		case 43:
				CHK_RDY
				MOS6510_Read(p, p->PC);
				p->SP = p->XR;
				break;
		//R // Lesen von PC-p->Adresse und verwerfen // p->AC=p->YR // Set p->SR(NZ)
		case 44:
				CHK_RDY
				MOS6510_Read(p, p->PC);
				p->AC = p->YR;
				MOS6510_SET_SR_NZ(p, p->AC);
				break;
		//W // p->AC nach SP+0x0100 schreiben // SP-1
		case 45:
				MOS6510_Write(p, p->SP + 0x0100, p->AC);
				p->SP--;
				break;
		//R // p->AC von SP+0x0100 lesen // SP+1
		case 46:
				CHK_RDY
				p->AC = MOS6510_Read(p, p->SP + 0x0100);
				p->SP++;
				break;
		//R // p->AC von SP+0x0100 lesen // Set p->SR(NZ)
		case 47:
				CHK_RDY
				p->AC = MOS6510_Read(p, p->SP + 0x0100);
				MOS6510_SET_SR_NZ(p, p->AC);
				break;
		//R // p->SR von SP+0x0100 lesen // SP+1
		case 48:
				CHK_RDY
				p->SR = MOS6510_Read(p, p->SP + 0x0100) | 32;
				p->SP++;
				break;
		//R // p->SR von SP+0x0100 lesen
		case 49:
				CHK_RDY
				p->SR = MOS6510_Read(p, p->SP + 0x0100) | 32;
				break;
		//R // p->TMPByte von PC-p->Adresse lesen // p->AC + p->TMPByte + Carry // PC+1
		case 50:
				CHK_RDY
				tmp1 = MOS6510_Read(p, p->PC);

				if (MOS6510_IF_DECIMAL(p))
				{
						tmp = (p->AC & 0xF) + (tmp1 & 0xF) + (p->SR & 0x1);
						if (tmp > 0x9) tmp += 0x6;
						if (tmp <= 0x0F) tmp = (tmp & 0xF) + (p->AC & 0xF0) + (tmp1 & 0xF0);
						else tmp = (tmp & 0xF) + (p->AC & 0xF0) + (tmp1 & 0xF0) + 0x10;
						MOS6510_SET_ZERO(p, ((p->AC + tmp1 + (p->SR & 0x1)) & 0xFF));
						MOS6510_SET_SIGN(p, tmp & 0x80);
						MOS6510_SET_OVERFLOW(p, ((p->AC ^ tmp) & 0x80) && !((p->AC ^ tmp1) & 0x80));
						if ((tmp & 0x1F0) > 0x90) tmp += 0x60;
						MOS6510_SET_CARRY(p, (tmp & 0xFF0) > 0xF0);
				}
				else
				{
						tmp = tmp1 + p->AC + (p->SR & 0x01);
						MOS6510_SET_SR_NZ(p, tmp & 0xff);
						MOS6510_SET_OVERFLOW(p, !((p->AC ^ tmp1) & 0x80) && ((p->AC ^ tmp) & 0x80));
						MOS6510_SET_CARRY(p, tmp > 0xFF);
				}
				p->AC = (unsigned char)tmp;
				p->PC++;
				break;

		//R // p->TMPByte von p->Adresse lesen // p->AC + p->TMPByte + Carry
		case 51:
				CHK_RDY
				tmp1 = MOS6510_Read(p, p->Adresse);

				if (MOS6510_IF_DECIMAL(p))
				{
						tmp = (p->AC & 0xF) + (tmp1 & 0xF) + (p->SR & 0x1);
						if (tmp > 0x9) tmp += 0x6;
						if (tmp <= 0x0F) tmp = (tmp & 0xF) + (p->AC & 0xF0) + (tmp1 & 0xF0);
						else tmp = (tmp & 0xF) + (p->AC & 0xF0) + (tmp1 & 0xF0) + 0x10;
						MOS6510_SET_ZERO(p, ((p->AC + tmp1 + (p->SR & 0x1)) & 0xFF));
						MOS6510_SET_SIGN(p, tmp & 0x80);
						MOS6510_SET_OVERFLOW(p, ((p->AC ^ tmp) & 0x80) && !((p->AC ^ tmp1) & 0x80));
						if ((tmp & 0x1F0) > 0x90) tmp += 0x60;
						MOS6510_SET_CARRY(p, (tmp & 0xFF0) > 0xF0);
				}
				else
				{
						tmp = tmp1 + p->AC + (p->SR & 0x01);
						MOS6510_SET_SR_NZ(p, tmp & 0xff);
						MOS6510_SET_OVERFLOW(p, !((p->AC ^ tmp1) & 0x80) && ((p->AC ^ tmp) & 0x80));
						MOS6510_SET_CARRY(p, tmp > 0xFF);
				}
				p->AC = (unsigned char)tmp;
				break;

		//R // p->TMPByte von p->Adresse lesen // p->Adresse Lo + p->XR
		case 52:
				CHK_RDY
				p->TMPByte = MOS6510_Read(p, p->Adresse);
				p->Adresse += p->XR;
				p->Adresse &= 0xFF;
				break;
		//R // p->Adresse Hi von PC-p->Adresse holen // p->Adresse+p->XR  // PC+1 //
		case 53:
				CHK_RDY
				SetAdresseHi(MOS6510_Read(p, p->PC));
				p->Adresse += p->XR;
				idxReg = p->XR;
				p->PC++;
				break;
		//R // p->Adresse Hi von PC-p->Adresse holen // p->Adresse+p->YR  // PC+1 //
		case 54:
				CHK_RDY
				SetAdresseHi(MOS6510_Read(p, p->PC));
				p->Adresse += p->YR;
				idxReg = p->YR;
				p->PC++;
				break;
		//R // p->TMPByte von p->Adresse lesen // p->AC + p->TMPByte + Carry // if(idxReg<p->Adresse Lo) p->MCT++
		case 55:
				CHK_RDY
				if((p->Adresse&0xFF)>=(idxReg))
				{
						tmp1 = MOS6510_Read(p, p->Adresse);
						if (MOS6510_IF_DECIMAL(p))
						{
								tmp = (p->AC & 0xF) + (tmp1 & 0xF) + (p->SR & 0x1);
								if (tmp > 0x9) tmp += 0x6;
								if (tmp <= 0x0F) tmp = (tmp & 0xF) + (p->AC & 0xF0) + (tmp1 & 0xF0);
								else tmp = (tmp & 0xF) + (p->AC & 0xF0) + (tmp1 & 0xF0) + 0x10;
								MOS6510_SET_ZERO(p, ((p->AC + tmp1 + (p->SR & 0x1)) & 0xFF));
								MOS6510_SET_SIGN(p, tmp & 0x80);
								MOS6510_SET_OVERFLOW(p, ((p->AC ^ tmp) & 0x80) && !((p->AC ^ tmp1) & 0x80));
								if ((tmp & 0x1F0) > 0x90) tmp += 0x60;
								MOS6510_SET_CARRY(p, (tmp & 0xFF0) > 0xF0);
						}
						else
						{
								tmp = tmp1 + p->AC + (p->SR & 0x01);
								MOS6510_SET_SR_NZ(p, tmp & 0xff);
								MOS6510_SET_OVERFLOW(p, !((p->AC ^ tmp1) & 0x80) && ((p->AC ^ tmp) & 0x80));
								MOS6510_SET_CARRY(p, tmp > 0xFF);
						}
						p->AC = (unsigned char)tmp;
						p->MCT++;
				}
				break;
		//R // p->TMPByte von PC-p->Adresse lesen // p->AC - p->TMPByte - Carry // PC+1
		case 56:
				CHK_RDY
				src=MOS6510_Read(p, p->PC);
				tmp2 = p->AC - src - ((p->SR & 0x01) ? 0 : 1);
				if (MOS6510_IF_DECIMAL(p))
				{
						tmp = (p->AC & 0xF) - (src & 0xF) - ((p->SR & 0x1) ? 0 : 1);
						if (tmp & 0x10) tmp = ((tmp - 6) & 0xF)| ((p->AC & 0xF0) - (src & 0xF0) - 0x10);
						else tmp = (tmp & 0xF) | ((p->AC & 0xF0) - (src & 0xF0));
						if (tmp & 0x100) tmp -= 0x60;
						MOS6510_SET_CARRY(p, tmp2 < 0x100);
						MOS6510_SET_SR_NZ(p, tmp2 & 0xFF);
						MOS6510_SET_OVERFLOW(p, ((p->AC ^ tmp2) & 0x80) && ((p->AC ^ src) & 0x80));
						p->AC = (unsigned char) tmp;
				}
				else
				{
						MOS6510_SET_SR_NZ(p, tmp2 & 0xff);
						MOS6510_SET_CARRY(p, tmp2 < 0x100);
						MOS6510_SET_OVERFLOW(p, ((p->AC ^ tmp2) & 0x80)
														 && ((p->AC ^ src) & 0x80));
						p->AC = (unsigned char) tmp2;
				}
				p->PC++;
				break;
		//R // p->TMPByte von p->Adresse lesen // p->AC - p->TMPByte - Carry
		case 57:
				CHK_RDY
				src=MOS6510_Read(p, p->Adresse);
				tmp2 = p->AC - src - ((p->SR & 0x01) ? 0 : 1);
				if (MOS6510_IF_DECIMAL(p))
				{
						tmp = (p->AC & 0xF) - (src & 0xF) - ((p->SR & 0x1) ? 0 : 1);
						if (tmp & 0x10) tmp = ((tmp - 6) & 0xF)| ((p->AC & 0xF0) - (src & 0xF0) - 0x10);
						else tmp = (tmp & 0xF) | ((p->AC & 0xF0) - (src & 0xF0));
						if (tmp & 0x100) tmp -= 0x60;
						MOS6510_SET_CARRY(p, tmp2 < 0x100);
						MOS6510_SET_SR_NZ(p, tmp2 & 0xFF);
						MOS6510_SET_OVERFLOW(p, ((p->AC ^ tmp2) & 0x80) && ((p->AC ^ src) & 0x80));
						p->AC = (unsigned char) tmp;
				}
				else
				{
						MOS6510_SET_SR_NZ(p, tmp2 & 0xff);
						MOS6510_SET_CARRY(p, tmp2 < 0x100);
						MOS6510_SET_OVERFLOW(p, ((p->AC ^ tmp2) & 0x80)
														 && ((p->AC ^ src) & 0x80));
						p->AC = (unsigned char) tmp2;
				}
				break;
		//R // p->TMPByte von p->Adresse lesen // p->AC - p->TMPByte - Carry // if(idxReg<p->Adresse Lo) p->MCT++
		case 58:
				CHK_RDY
				if((p->Adresse&0xFF)>=(idxReg))
				{
						src=MOS6510_Read(p, p->Adresse);
						tmp2 = p->AC - src - ((p->SR & 0x01) ? 0 : 1);
						if (MOS6510_IF_DECIMAL(p))
						{
								tmp = (p->AC & 0xF) - (src & 0xF) - ((p->SR & 0x1) ? 0 : 1);
								if (tmp & 0x10) tmp = ((tmp - 6) & 0xF)| ((p->AC & 0xF0) - (src & 0xF0) - 0x10);
								else tmp = (tmp & 0xF) | ((p->AC & 0xF0) - (src & 0xF0));
								if (tmp & 0x100) tmp -= 0x60;
								MOS6510_SET_CARRY(p, tmp2 < 0x100);
								MOS6510_SET_SR_NZ(p, tmp2 & 0xFF);
								MOS6510_SET_OVERFLOW(p, ((p->AC ^ tmp2) & 0x80) && ((p->AC ^ src) & 0x80));
								p->AC = (unsigned char) tmp;
						}
						else
						{
								MOS6510_SET_SR_NZ(p, tmp2 & 0xff);
								MOS6510_SET_CARRY(p, tmp2 < 0x100);
								MOS6510_SET_OVERFLOW(p, ((p->AC ^ tmp2) & 0x80)
																 && ((p->AC ^ src) & 0x80));
								p->AC = (unsigned char) tmp2;
						}
						p->MCT++;
				}
				break;
		//R // p->TMPByte von SP+0x0100 holen
		case 59:
				CHK_RDY
				p->TMPByte=MOS6510_Read(p, p->SP + 0x0100);
				break;
		//W // PC-p->Adresse Hi nach SP+0x0100 schreiben // SP-1
		case 60:
				MOS6510_Write(p, p->SP+0x0100,GetPCHi);
				p->SP--;
				break;
		//W // PC-p->Adresse Lo nach SP+0x0100 schreiben // SP-1
		case 61:
				MOS6510_Write(p, p->SP+0x0100,GetPCLo);
				p->SP--;
				break;
		//R // p->TMPByte von SP+0x0100 holen // SP+1
		case 62:
				CHK_RDY
				p->TMPByte = MOS6510_Read(p, p->SP+0x0100);
				p->SP++;
				break;
		//R // PC-p->Adresse Lo von SP+0x0100 holen // SP+1
		case 63:
				CHK_RDY
				SetPCLo(MOS6510_Read(p, p->SP+0x0100));
				p->SP++;
				break;
		//R // PC-p->Adresse Hi von SP+0x0100 holen
		case 64:
				CHK_RDY
				SetPCHi(MOS6510_Read(p, p->SP+0x0100));
				break;
		//R // p->TMPByte von PC-p->Adresse laden // PC+1
		case 65:
				CHK_RDY
				p->TMPByte = MOS6510_Read(p, p->PC);
				p->PC++;
				break;
		//R // p->TMPByte von PC-p->Adresse lesen // p->AC and p->TMPByte // Set p->SR(NZ) // PC+1
		case 66:
				CHK_RDY
				p->TMPByte=MOS6510_Read(p, p->PC);
				p->AC &= p->TMPByte;
				MOS6510_SET_SR_NZ(p, p->AC);
				p->PC++;
				break;
		//R // p->TMPByte von p->Adresse lesen // p->AC and p->TMPByte // Set p->SR(NZ)
		case 67:
				CHK_RDY
				p->TMPByte=MOS6510_Read(p, p->Adresse);
				p->AC &= p->TMPByte;
				MOS6510_SET_SR_NZ(p, p->AC);
				break;
		//R // p->TMPByte von p->Adresse lesen // p->AC and p->TMPByte // Set p->SR(NZ) // if(idxReg<p->Adresse Lo) p->MCT++
		case 68:
				CHK_RDY
				if((p->Adresse&0xFF)>=(idxReg))
				{
						p->TMPByte=MOS6510_Read(p, p->Adresse);
						p->AC &= p->TMPByte;
						MOS6510_SET_SR_NZ(p, p->AC);
						p->MCT++;
				}
				break;
		//R // p->TMPByte von p->Adresse lesen // CarrayFalg=0
		case 69:
				CHK_RDY
				p->TMPByte = MOS6510_Read(p, p->PC);
				p->SR &= 0xFE;
				break;
		//R // p->TMPByte von p->Adresse lesen // DezimalFalg=0
		case 70:
				CHK_RDY
				p->TMPByte = MOS6510_Read(p, p->PC);
				p->SR &= 0xF7;
				break;
		//R // p->TMPByte von p->Adresse lesen // InterruptFalg=0
		case 71:
				CHK_RDY
				p->TMPByte = MOS6510_Read(p, p->PC);
				p->irq_delay_sr_value = p->SR;
				p->SR &= 0xFB;
				p->irq_delay = true;
				break;
		//R // p->TMPByte von p->Adresse lesen // OverflowFalg=0
		case 72:
				CHK_RDY
				p->TMPByte = MOS6510_Read(p, p->PC);
				p->SR &= 0xBF;
				break;
		//R // p->TMPByte von p->Adresse lesen // CarrayFalg=1
		case 73:
				CHK_RDY
				p->TMPByte = MOS6510_Read(p, p->PC);
				p->SR |= 0x01;
				break;
		//R // p->TMPByte von p->Adresse lesen // DezimalFalg=1
		case 74:
				CHK_RDY
				p->TMPByte = MOS6510_Read(p, p->PC);
				p->SR |= 0x08;
				break;
		//R // p->TMPByte von p->Adresse lesen // InterruptFalg=1
		case 75:
				CHK_RDY
				p->TMPByte = MOS6510_Read(p, p->PC);
				p->irq_delay_sr_value = p->SR;
				p->SR |= 0x04;
				p->irq_delay = true;
				break;
		//R // p->TMPByte von p->Adresse lesen // BIT Operation
		case 76:
				CHK_RDY
				p->TMPByte = MOS6510_Read(p, p->Adresse);
				MOS6510_SET_ZERO(p, p->AC & p->TMPByte);
				p->SR = (p->TMPByte & 0xC0) | (p->SR & 0x3F);
				break;
		//W // p->AC nach p->Adresse schreiben
		case 77:
				MOS6510_Write(p, p->Adresse,p->AC);
				break;
		//W // p->XR nach p->Adresse schreiben
		case 78:
				MOS6510_Write(p, p->Adresse,p->XR);
				break;
		//W // p->YR nach p->Adresse schreiben
		case 79:
				MOS6510_Write(p, p->Adresse,p->YR);
				break;
		//R // p->AC von PC-p->Adresse lesen // Set p->SR(NZ) // PC+1
		case 80:
				CHK_RDY
				p->AC = MOS6510_Read(p, p->PC);
				MOS6510_SET_SR_NZ(p, p->AC);
				p->PC++;
				break;
		//R // p->AC von p->Adresse lesen // Set p->SR(NZ)
		case 81:
				CHK_RDY
				p->AC = MOS6510_Read(p, p->Adresse);
				MOS6510_SET_SR_NZ(p, p->AC);
				break;
		//R // p->AC von p->Adresse lesen // Set p->SR(NZ) // if(idxReg<p->Adresse Lo) p->MCT++
		case 82:
				CHK_RDY
				if((p->Adresse&0xFF)>=(idxReg))
				{
						p->TMPByte=MOS6510_Read(p, p->Adresse);
						p->AC = p->TMPByte;
						MOS6510_SET_SR_NZ(p, p->AC);
						p->MCT++;
				}
				break;
		//R // p->XR von PC-p->Adresse lesen // Set p->SR(NZ) // PC+1
		case 83:
				CHK_RDY
				p->XR = MOS6510_Read(p, p->PC);
				MOS6510_SET_SR_NZ(p, p->XR);
				p->PC++;
				break;
		//R // p->XR von p->Adresse lesen // Set p->SR(NZ)
		case 84:
				CHK_RDY
				p->XR = MOS6510_Read(p, p->Adresse);
				MOS6510_SET_SR_NZ(p, p->XR);
				break;
		//R // p->XR von p->Adresse lesen // Set p->SR(NZ) // if(idxReg<p->Adresse Lo) p->MCT++
		case 85:
				CHK_RDY
				if((p->Adresse&0xFF)>=(idxReg))
				{
						p->TMPByte=MOS6510_Read(p, p->Adresse);
						p->XR = p->TMPByte;
						MOS6510_SET_SR_NZ(p, p->XR);
						p->MCT++;
				}
				break;
		//R // p->YR von PC-p->Adresse lesen // Set p->SR(NZ) // PC+1
		case 86:
				CHK_RDY
				p->YR = MOS6510_Read(p, p->PC);
				MOS6510_SET_SR_NZ(p, p->YR);
				p->PC++;
				break;
		//R // p->YR von p->Adresse lesen // Set p->SR(NZ)
		case 87:
				CHK_RDY
				p->YR = MOS6510_Read(p, p->Adresse);
				MOS6510_SET_SR_NZ(p, p->YR);
				break;
		//R // p->YR von p->Adresse lesen // Set p->SR(NZ) // if(idxReg<p->Adresse Lo) p->MCT++
		case 88:
				CHK_RDY
				if((p->Adresse&0xFF)>=(idxReg))
				{
						p->TMPByte=MOS6510_Read(p, p->Adresse);
						p->YR = p->TMPByte;
						MOS6510_SET_SR_NZ(p, p->YR);
						p->MCT++;
				}
				break;
		//R // p->TMPByte von p->Adresse lesen // p->XR+1 // Set p->SR(NZ)
		case 89:
				CHK_RDY
				p->TMPByte = MOS6510_Read(p, p->PC);
				p->XR ++;
				MOS6510_SET_SR_NZ(p, p->XR);
				break;
		//R // p->TMPByte von p->Adresse lesen // p->YR+1 // Set p->SR(NZ)
		case 90:
				CHK_RDY
				p->TMPByte = MOS6510_Read(p, p->PC);
				p->YR ++;
				MOS6510_SET_SR_NZ(p, p->YR);
				break;
		//R // p->TMPByte von p->Adresse lesen // p->XR-1 // Set p->SR(NZ)
		case 91:
				CHK_RDY
				p->TMPByte = MOS6510_Read(p, p->PC);
				p->XR --;
				MOS6510_SET_SR_NZ(p, p->XR);
				break;
		//R // p->TMPByte von p->Adresse lesen // p->YR-1 // Set p->SR(NZ)
		case 92:
				CHK_RDY
				p->TMPByte = MOS6510_Read(p, p->PC);
				p->YR --;
				MOS6510_SET_SR_NZ(p, p->YR);
				break;
		//R // Illegale Opcode //
		case 93:
				//// Wird nie angesprungen !!! ////
				break;
		//R // PC LO von p->Adresse lesen // p->AdresseLo+1
		case 94:
				CHK_RDY
				SetPCLo(MOS6510_Read(p, p->Adresse));
				p->Adresse = (p->Adresse&0xFF00)|((p->Adresse+1)&0x00FF);
				break;
		//R // PC HI von p->Adresse lesen
		case 95:
				CHK_RDY
				SetPCHi(MOS6510_Read(p, p->Adresse));
				break;
		//R // PC LO von $FFFC lesen
		case 96:
				CHK_RDY
				SetPCLo(MOS6510_Read(p, 0xFFFC));
				break;
		//R // PC HI von $FFFD lesen
		case 97:
				CHK_RDY
				SetPCHi(MOS6510_Read(p, 0xFFFD));
				break;
		//R // p->TMPByte von PC-p->Adresse lesen // p->AC - p->TMPByte (p->AC wird nicht verÃ€ndert) // Set p->SR(NZC) // PC+1
		case 98:
				CHK_RDY
				p->TMPByte=MOS6510_Read(p, p->PC);
				tmp = p->AC - p->TMPByte;
				MOS6510_SET_CARRY(p, tmp < 0x100);
				MOS6510_SET_SIGN(p, tmp);
				MOS6510_SET_ZERO(p, tmp &= 0xFF);
				p->PC++;
				break;
		//R // p->TMPByte von p->Adresse lesen // p->AC - p->TMPByte (p->AC wird nicht verÃ€ndert) // Set p->SR(NZC)
		case 99:
				CHK_RDY
				p->TMPByte=MOS6510_Read(p, p->Adresse);
				tmp = p->AC - p->TMPByte;
				MOS6510_SET_CARRY(p, tmp < 0x100);
				MOS6510_SET_SIGN(p, tmp);
				MOS6510_SET_ZERO(p, tmp &= 0xFF);
				break;
		//R // p->TMPByte von p->Adresse lesen // p->AC - p->TMPByte (p->AC wird nicht verÃ€ndert) // if(idxReg<p->Adresse Lo) p->MCT++
		case 100:
				CHK_RDY
				if((p->Adresse&0xFF)>=(idxReg))
				{
						p->TMPByte=MOS6510_Read(p, p->Adresse);
						tmp = p->AC - p->TMPByte;
						MOS6510_SET_CARRY(p, tmp < 0x100);
						MOS6510_SET_SIGN(p, tmp);
						MOS6510_SET_ZERO(p, tmp &= 0xFF);
						p->MCT++;
				}
				break;
		//R // p->TMPByte von PC-p->Adresse lesen // p->XR - p->TMPByte (p->XR wird nicht verÃ€ndert) // Set p->SR(NZC) // PC+1
		case 101:
				CHK_RDY
				p->TMPByte=MOS6510_Read(p, p->PC);
				tmp = p->XR - p->TMPByte;
				MOS6510_SET_CARRY(p, tmp < 0x100);
				MOS6510_SET_SIGN(p, tmp);
				MOS6510_SET_ZERO(p, tmp &= 0xFF);
				p->PC++;
				break;
		//R // p->TMPByte von p->Adresse lesen // p->XR - p->TMPByte (p->XR wird nicht verÃ€ndert) // Set p->SR(NZC)
		case 102:
				CHK_RDY
				p->TMPByte=MOS6510_Read(p, p->Adresse);
				tmp = p->XR - p->TMPByte;
				MOS6510_SET_CARRY(p, tmp < 0x100);
				MOS6510_SET_SIGN(p, tmp);
				MOS6510_SET_ZERO(p, tmp &= 0xFF);
				break;
		//R // p->TMPByte von PC-p->Adresse lesen // p->YR - p->TMPByte (p->XR wird nicht verÃ€ndert) // Set p->SR(NZC) // PC+1
		case 103:
				CHK_RDY
				p->TMPByte=MOS6510_Read(p, p->PC);
				tmp = p->YR - p->TMPByte;
				MOS6510_SET_CARRY(p, tmp < 0x100);
				MOS6510_SET_SIGN(p, tmp);
				MOS6510_SET_ZERO(p, tmp &= 0xFF);
				p->PC++;
				break;
		//R // p->TMPByte von p->Adresse lesen // p->YR - p->TMPByte (p->XR wird nicht verÃ€ndert) // Set p->SR(NZC)
		case 104:
				CHK_RDY
				p->TMPByte=MOS6510_Read(p, p->Adresse);
				tmp = p->YR - p->TMPByte;
				MOS6510_SET_CARRY(p, tmp < 0x100);
				MOS6510_SET_SIGN(p, tmp);
				MOS6510_SET_ZERO(p, tmp &= 0xFF);
				break;
		//R // p->TMPByte von PC-p->Adresse lesen // p->AC XOR p->TMPByte // Set p->SR(NZC) // PC+1
		case 105:
				CHK_RDY
				p->TMPByte=MOS6510_Read(p, p->PC);
				p->AC^=p->TMPByte;
				MOS6510_SET_SR_NZ(p, p->AC);
				p->PC++;
				break;
		//R // p->TMPByte von p->Adresse lesen // p->AC XOR p->TMPByte // Set p->SR(NZC)
		case 106:
				CHK_RDY
				p->TMPByte=MOS6510_Read(p, p->Adresse);
				p->AC^=p->TMPByte;
				MOS6510_SET_SR_NZ(p, p->AC);
				break;
		//R // p->TMPByte von p->Adresse lesen // p->AC XOR p->TMPByte // if(idxReg<p->Adresse Lo) p->MCT++
		case 107:
				CHK_RDY
				if((p->Adresse&0xFF)>=(idxReg))
				{
						p->TMPByte=MOS6510_Read(p, p->Adresse);
						p->AC^=p->TMPByte;
						MOS6510_SET_SR_NZ(p, p->AC);
						p->MCT++;
				}
				break;
		//R // p->TMPByte von PC-p->Adresse holen // p->AC>>1 // Set p->SR(NZC)
		case 108:
				CHK_RDY
				p->TMPByte = MOS6510_Read(p, p->PC);
				MOS6510_SET_CARRY(p, p->AC&0x01);
				p->AC >>= 1;
				MOS6510_SET_SIGN(p, 0);
				MOS6510_SET_ZERO(p, p->AC);
				break;
		//W // p->TMPByte nach p->Adresse schreiben // p->TMPByte>>1 // Set p->SR(NZC)
		case 109:
				MOS6510_Write(p, p->Adresse,p->TMPByte);
				MOS6510_SET_CARRY(p, p->TMPByte&0x01);
				p->TMPByte >>= 1;
				MOS6510_SET_SIGN(p, 0);
				MOS6510_SET_ZERO(p, p->TMPByte);
				break;
		//R // p->TMPByte von PC-p->Adresse holen // C<-p->AC<<1<-C // Set p->SR(NZC)
		case 110:
				CHK_RDY
				p->TMPByte = MOS6510_Read(p, p->PC);
				tmp = p->AC;
				tmp <<= 1;
				if (MOS6510_IF_CARRY(p)) tmp |= 0x1;
				MOS6510_SET_CARRY(p, tmp > 0xff);
				tmp &= 0xff;
				MOS6510_SET_SIGN(p, tmp);
				MOS6510_SET_ZERO(p, tmp);
				p->AC = tmp;
				break;
		//W // p->TMPByte nach p->Adresse schreiben // C<-p->TMPByte<<1<-C // Set p->SR(NZC)
		case 111:
				MOS6510_Write(p, p->Adresse,p->TMPByte);
				tmp = p->TMPByte;
				tmp <<= 1;
				if (MOS6510_IF_CARRY(p)) tmp |= 0x1;
				MOS6510_SET_CARRY(p, tmp > 0xff);
				tmp &= 0xff;
				MOS6510_SET_SIGN(p, tmp);
				MOS6510_SET_ZERO(p, tmp);
				p->TMPByte = tmp;
				break;
		//R // p->TMPByte von PC-p->Adresse holen // C->p->AC>>1->C // Set p->SR(NZC)
		case 112:
				CHK_RDY
				p->TMPByte = MOS6510_Read(p, p->PC);
				carry_tmp=MOS6510_IF_CARRY(p);
				MOS6510_SET_CARRY(p, p->AC & 0x01);
				p->AC >>= 1;
				if(carry_tmp) p->AC |= 0x80;
				MOS6510_SET_SIGN(p, p->AC);
				MOS6510_SET_ZERO(p, p->AC);
				break;
		//W // p->TMPByte nach p->Adresse schreiben // C->p->TMPByte>>1->C // Set p->SR(NZC)
		case 113:
				MOS6510_Write(p, p->Adresse,p->TMPByte);
				carry_tmp=MOS6510_IF_CARRY(p);
				MOS6510_SET_CARRY(p, p->TMPByte & 0x01);
				p->TMPByte >>= 1;
				if(carry_tmp) p->TMPByte |= 0x80;
				MOS6510_SET_SIGN(p, p->TMPByte);
				MOS6510_SET_ZERO(p, p->TMPByte);
				break;
		//W // p->TMPByte nach p->Adresse schreiben // p->TMPByte+1 // Set p->SR(NZ)
		case 114:
				MOS6510_Write(p, p->Adresse,p->TMPByte);
				p->TMPByte++;
				MOS6510_SET_SR_NZ(p, p->TMPByte);
				break;
		//W // p->TMPByte nach p->Adresse schreiben // p->TMPByte-1 // Set p->SR(NZ)
		case 115:
				MOS6510_Write(p, p->Adresse,p->TMPByte);
				p->TMPByte--;
				MOS6510_SET_SR_NZ(p, p->TMPByte);
				break;
		//W // p->SR nach 0x100+SP schreiben // SP-- // IFlag setzen // BFlag lÃ¶schen
		case 116:
				p->SR&=239;
				MOS6510_Write(p, 0x0100 + p->SP, p->SR);
				p->SP--;
				p->SR|=4;
				break;
		//R // PC Lo von 0xFFFA holen
		case 117:
				CHK_RDY
				SetPCLo(MOS6510_Read(p, 0xFFFA));
				break;
		//R // PC Hi von 0xFFFB holen
		case 118:
				CHK_RDY
				SetPCHi(MOS6510_Read(p, 0xFFFB));
				break;
		//R // p->TMPByte von p->Adresse holen // Fix p->Adresse Hi p->MCT+1
		case 119:
				CHK_RDY
				if((p->Adresse&0xFF)>=(idxReg))
				{
						p->TMPByte = MOS6510_Read(p, p->Adresse);
						p->MCT++;
				}
				break;
		//W // p->TMPByte nach p->Adresse schreiben // Illegal [SLO]
		case 120:
				MOS6510_Write(p, p->Adresse,p->TMPByte);
				MOS6510_SET_CARRY(p, p->TMPByte&0x80);	// ASL MEMORY
				p->TMPByte <<= 1;
				p->AC |= p->TMPByte;				// ORA
				MOS6510_SET_SR_NZ(p, p->AC);
				break;
		//W // p->TMPByte nach p->Adresse schreiben // Illegal [RLA]
		case 121:
				MOS6510_Write(p, p->Adresse,p->TMPByte);
				tmp = p->TMPByte;
				tmp <<= 1;					// ROL
				if (MOS6510_IF_CARRY(p)) tmp |= 0x1;
				MOS6510_SET_CARRY(p, tmp > 0xFF);
				tmp &= 0xFF;
				p->TMPByte = tmp;
				p->AC &= p->TMPByte;				// AND
				MOS6510_SET_SR_NZ(p, p->AC);
				break;
		//W // p->TMPByte nach p->Adresse schreiben // Illegal [p->SRE]
		case 122:
				MOS6510_Write(p, p->Adresse,p->TMPByte);
				MOS6510_SET_CARRY(p, p->TMPByte & 0x01);	// Lp->SR
				p->TMPByte >>= 1;
				p->AC ^= p->TMPByte;				// EOR
				MOS6510_SET_SR_NZ(p, p->AC);
				break;
		//W // p->TMPByte nach p->Adresse schreiben // Illegal [RRA]
		case 123:
				MOS6510_Write(p, p->Adresse,p->TMPByte);

				carry_tmp = MOS6510_IF_CARRY(p);		// ROR
				MOS6510_SET_CARRY(p, p->TMPByte & 0x01);
				p->TMPByte >>= 1;
				if(carry_tmp) p->TMPByte |= 0x80;

				tmp1 = p->TMPByte;				// ADC
				if (MOS6510_IF_DECIMAL(p))
				{
						tmp = (p->AC & 0xF) + (tmp1 & 0xF) + (p->SR & 0x1);
						if (tmp > 0x9) tmp += 0x6;
						if (tmp <= 0x0F) tmp = (tmp & 0xF) + (p->AC & 0xF0) + (tmp1 & 0xF0);
						else tmp = (tmp & 0xF) + (p->AC & 0xF0) + (tmp1 & 0xF0) + 0x10;
						MOS6510_SET_ZERO(p, ((p->AC + tmp1 + (p->SR & 0x1)) & 0xFF));
						MOS6510_SET_SIGN(p, tmp & 0x80);
						MOS6510_SET_OVERFLOW(p, ((p->AC ^ tmp) & 0x80) && !((p->AC ^ tmp1) & 0x80));
						if ((tmp & 0x1F0) > 0x90) tmp += 0x60;
						MOS6510_SET_CARRY(p, (tmp & 0xFF0) > 0xF0);
				}
				else
				{
						tmp = tmp1 + p->AC + (p->SR & 0x01);
						MOS6510_SET_SR_NZ(p, tmp & 0xff);
						MOS6510_SET_OVERFLOW(p, !((p->AC ^ tmp1) & 0x80) && ((p->AC ^ tmp) & 0x80));
						MOS6510_SET_CARRY(p, tmp > 0xFF);
				}
				p->AC = (unsigned char)tmp;
				break;
		//W // p->TMPByte nach p->Adresse schreiben // Illegal [DCP]
		case 124:
				MOS6510_Write(p, p->Adresse,p->TMPByte);
				p->TMPByte--;					//DEC

				tmp = p->AC - p->TMPByte;			//CMP
				MOS6510_SET_CARRY(p, tmp < 0x100);
				MOS6510_SET_SIGN(p, tmp);
				MOS6510_SET_ZERO(p, tmp &= 0xFF);
				break;
		//W // p->TMPByte nach p->Adresse schreiben // Illegal [ISB]
		case 125:
				MOS6510_Write(p, p->Adresse,p->TMPByte);
				p->TMPByte++;					//INC

				src=p->TMPByte;				//SBC
				tmp2 = p->AC - src - ((p->SR & 0x01) ? 0 : 1);
				if (MOS6510_IF_DECIMAL(p))
				{
						tmp = (p->AC & 0xF) - (src & 0xF) - ((p->SR & 0x1) ? 0 : 1);
						if (tmp & 0x10) tmp = ((tmp - 6) & 0xF)| ((p->AC & 0xF0) - (src & 0xF0) - 0x10);
						else tmp = (tmp & 0xF) | ((p->AC & 0xF0) - (src & 0xF0));
						if (tmp & 0x100) tmp -= 0x60;
						MOS6510_SET_CARRY(p, tmp2 < 0x100);
						MOS6510_SET_SR_NZ(p, tmp2 & 0xFF);
						MOS6510_SET_OVERFLOW(p, ((p->AC ^ tmp2) & 0x80) && ((p->AC ^ src) & 0x80));
						p->AC = (unsigned char) tmp;
				}
				else
				{
						MOS6510_SET_SR_NZ(p, tmp2 & 0xff);
						MOS6510_SET_CARRY(p, tmp2 < 0x100);
						MOS6510_SET_OVERFLOW(p, ((p->AC ^ tmp2) & 0x80)
														 && ((p->AC ^ src) & 0x80));
						p->AC = (unsigned char) tmp2;
				}
				break;
		//R // p->AC von p->Adresse lesen // p->AC -> p->XR // Set p->SR(NZ) // Illegal [LAX]
		case 126:
				CHK_RDY
				p->AC = MOS6510_Read(p, p->Adresse);
				p->XR = p->AC;
				MOS6510_SET_SR_NZ(p, p->AC);
				break;
		//R // p->AC von p->Adresse lesen // p->AC -> p->XR // Set p->SR(NZ) // if(idxReg<p->Adresse Lo) p->MCT++ // Illegal [LAX]
		case 127:
				CHK_RDY
				if((p->Adresse&0xFF)>=(idxReg))
				{
						p->TMPByte=MOS6510_Read(p, p->Adresse);
						p->AC = p->TMPByte;
						p->XR = p->AC;
						MOS6510_SET_SR_NZ(p, p->AC);
						p->MCT++;
				}
				break;
		//W // p->TMPByte = p->AC & p->XR // p->TMPByte nach p->Adresse schreiben // Set p->SR(NZ) // Illegal [SAX]
		case 128:
				p->TMPByte = p->AC & p->XR;
				MOS6510_Write(p, p->Adresse,p->TMPByte);
				break;
		//R // Illegal [Ap->SR]
		case 129:
				CHK_RDY
				p->AC &= MOS6510_Read(p, p->PC);				// AND

				MOS6510_SET_CARRY(p, p->AC & 0x01);		// Lp->SR
				p->AC >>= 1;
				MOS6510_SET_SIGN(p, 0);
				MOS6510_SET_ZERO(p, p->AC);
				p->PC++;
				break;
		//R // Illegal [ARR]
		case 130:
				CHK_RDY
				tmp2 = MOS6510_Read(p, p->PC) & p->AC;
				p->AC = ((p->SR & 0x01) ? (tmp2 >> 1) | 0x80 : tmp2 >> 1);
				if (!MOS6510_IF_DECIMAL(p))
				{
						MOS6510_SET_SR_NZ(p, p->AC);
						MOS6510_SET_CARRY(p, p->AC & 0x40);
						MOS6510_SET_OVERFLOW(p, !!((p->AC & 0x40) ^ ((p->AC & 0x20) << 1)));
				}
				else
				{
						MOS6510_SET_SIGN(p, (p->SR & 0x01) ? 0x80 : 0);
						MOS6510_SET_ZERO(p, p->AC);
						MOS6510_SET_OVERFLOW(p, !!((tmp2 ^ p->AC) & 0x40));
						if ((tmp2 & 0x0F) + (tmp2 & 0x01) > 5) p->AC = (p->AC & 0xF0) | ((p->AC + 6) & 0x0F);
						MOS6510_SET_CARRY(p, ((tmp2 + (tmp2 & 0x10)) & 0x1F0)>0x50);
						if(MOS6510_IF_CARRY(p)) p->AC += 0x60;
				}
				p->PC++;
				break;
		//R // Illegal [ANE]
		case 131:
				CHK_RDY
				p->TMPByte = MOS6510_Read(p, p->PC);
				p->AC = (p->AC | 0xEE) & p->XR & p->TMPByte;
				MOS6510_SET_SR_NZ(p, p->AC);
				p->PC++;
				break;
		//R // Illegal [LXA]
		case 132:
				CHK_RDY
				p->TMPByte = MOS6510_Read(p, p->PC);
				p->AC = p->XR = ((p->AC | 0xEE) & p->TMPByte);
				MOS6510_SET_SR_NZ(p, p->AC);
				p->PC++;
				break;
		//R // Illegal [SBX]
		case 133:
				CHK_RDY
				tmp = MOS6510_Read(p, p->PC);
				tmp = (p->AC & p->XR) - tmp;
				MOS6510_SET_CARRY(p, tmp < 0x100);
				p->XR = tmp & 0xFF;
				MOS6510_SET_SR_NZ(p, p->XR);
				p->PC++;
				break;
		//W // Illegal [SHY]
		case 134:
				if((p->Adresse & 0xFF) < p->XR)
				{
					// page boundary crossing
					p->axa_byte = (p->Adresse >> 8) & p->YR;
					p->Adresse = (p->Adresse & 0xFF) | p->axa_byte << 8;
				}
				else
					p->axa_byte = p->YR & ((p->Adresse >> 8) + 1);
				if(p->shxy_dma)
				{
					p->axa_byte = p->YR;
					p->shxy_dma = false;
				}
				MOS6510_Write(p, p->Adresse, p->axa_byte);
				break;
		//W // Illegal [SHX]
		case 135:
			if((p->Adresse & 0xFF) < p->YR)
			{
				// page boundary crossing
				p->axa_byte = (p->Adresse >> 8) & p->XR;
				p->Adresse = (p->Adresse & 0xFF) | p->axa_byte << 8;
			}
			else
				p->axa_byte = p->XR & ((p->Adresse >> 8) + 1);
			if(p->shxy_dma)
			{
				p->axa_byte = p->XR;
				p->shxy_dma = false;
			}
			MOS6510_Write(p, p->Adresse, p->axa_byte);
			break;
		//W // Illegal [SHA]
		case 136:
				MOS6510_Write(p, p->Adresse, p->AC & p->XR & ((p->Adresse >> 8) + 1));
				break;
		//W // Illegal [SHS]
		case 137:
				p->SP = p->AC & p->XR;
				MOS6510_Write(p, p->Adresse, p->AC & p->XR & ((p->Adresse >> 8) + 1));
				break;
		//R // Illegal [ANC]
		case 138:
				CHK_RDY
				p->AC &= MOS6510_Read(p, p->PC);
				MOS6510_SET_SR_NZ(p, p->AC);
				MOS6510_SET_CARRY(p, p->SR&0x80);
				p->PC++;
				break;
		//R // Illegal [LAE]
		case 139:
				CHK_RDY
				p->TMPByte = MOS6510_Read(p, p->Adresse);
				p->AC = p->XR = p->SP = p->SP & (p->TMPByte);
				MOS6510_SET_SR_NZ(p, p->AC);
				break;
		//R // Illegal [LAE]
		case 140:
				CHK_RDY
				if((p->Adresse&0xFF)>=(idxReg))
				{
						p->TMPByte=MOS6510_Read(p, p->Adresse);
						p->AC = p->XR = p->SP = p->SP & (p->TMPByte);
						MOS6510_SET_SR_NZ(p, p->AC);
						p->MCT++;
				}
				break;
		}
		p->MCT++;

		if(*p->MCT == 0)
		{
			p->AktOpcodePC = p->PC;
			if(p->ResetReadyAdr == p->PC)
			{
				*p->ResetReady = true;
			}
			return true;
		}
		else return false;
	}
	return false;
}

static void MOS6510_Phi1(MOS6510* p)
{
	if(p->irq_is_low_pegel)
		p->irq_is_active = true;
	else
        p->irq_is_active = false;

	if(p->nmi_fall_edge)
		p->nmi_is_active = true;
}

static const uint8_t CPU_OPC_INFO[256]={\
0*16+6+0,7*16+5+0,0*16+0+8,7*16+7+8,3*16+2+8,3*16+2+0,3*16+4+0,3*16+4+8,0*16+2+0,1*16+1+0,0*16+1+0,1*16+1+8,2*16+3+8,2*16+3+0,2*16+5+0,2*16+5+8\
,9*16+1+0,8*16+4+0,0*16+0+8,8*16+7+8,6*16+3+8,6*16+3+0,6*16+5+0,6*16+5+8,0*16+1+0,5*16+3+0,0*16+1+8,5*16+6+8,4*16+3+8,4*16+3+0,4*16+6+0,4*16+6+8\
,2*16+5+0,7*16+5+0,0*16+0+8,7*16+7+8,3*16+2+0,3*16+2+0,3*16+4+0,3*16+4+8,0*16+3+0,1*16+1+0,0*16+1+0,1*16+1+8,2*16+3+0,2*16+3+0,2*16+5+0,2*16+5+8\
,9*16+1+0,8*16+4+0,0*16+0+8,8*16+7+8,6*16+3+8,6*16+3+0,6*16+5+0,6*16+5+8,0*16+1+0,5*16+3+0,0*16+1+8,5*16+6+8,4*16+3+8,4*16+3+0,4*16+6+0,4*16+6+8\
,0*16+5+0,7*16+5+0,0*16+0+8,7*16+7+8,3*16+2+8,3*16+2+0,3*16+4+0,3*16+4+8,0*16+2+0,1*16+1+0,0*16+1+0,1*16+1+8,2*16+2+0,2*16+4+0,2*16+5+0,2*16+5+8\
,9*16+1+0,8*16+4+0,0*16+0+8,8*16+7+8,6*16+3+8,6*16+3+0,6*16+5+0,6*16+5+8,0*16+1+0,5*16+3+0,0*16+1+8,5*16+6+8,4*16+3+8,4*16+3+0,4*16+6+0,4*16+6+8\
,0*16+5+0,7*16+5+0,0*16+0+8,7*16+7+8,3*16+2+8,3*16+2+0,3*16+4+0,3*16+4+8,0*16+2+0,1*16+1+0,0*16+1+0,1*16+1+8,10*16+4+0,2*16+3+0,2*16+5+0,2*16+5+8\
,9*16+1+0,8*16+4+0,0*16+0+8,8*16+7+8,6*16+3+8,6*16+3+0,6*16+5+0,6*16+5+8,0*16+1+0,5*16+3+0,0*16+1+8,5*16+6+8,4*16+3+8,4*16+3+0,4*16+6+0,4*16+6+8\
,1*16+1+8,7*16+5+0,0*16+1+8,7*16+5+8,3*16+2+0,3*16+2+0,3*16+2+0,3*16+2+8,0*16+1+0,1*16+1+8,0*16+1+0,1*16+1+8,2*16+3+0,2*16+3+0,2*16+3+0,2*16+3+8\
,9*16+1+0,8*16+5+0,0*16+0+8,4*16+5+8,6*16+1+0,6*16+3+0,11*16+3+0,11*16+3+8,0*16+1+0,5*16+1+0,0*16+1+0,5*16+4+8,5*16+4+8,4*16+4+0,4*16+4+8,5*16+4+8\
,1*16+1+0,7*16+5+0,1*16+1+0,7*16+5+8,3*16+2+0,3*16+2+0,3*16+2+0,3*16+2+8,0*16+1+0,1*16+1+0,0*16+1+0,1*16+1+8,2*16+3+0,2*16+3+0,2*16+3+0,2*16+3+8\
,9*16+1+0,8*16+4+0,0*16+0+8,8*16+4+8,6*16+1+0,6*16+3+0,11*16+3+0,11*16+3+8,0*16+1+0,5*16+3+0,0*16+1+0,5*16+3+8,4*16+3+0,4*16+3+0,5*16+3+0,5*16+3+8\
,1*16+1+0,7*16+5+0,1*16+1+8,7*16+7+8,3*16+2+0,3*16+2+0,3*16+4+0,3*16+4+8,0*16+1+0,1*16+1+0,0*16+1+0,1*16+1+8,2*16+3+0,2*16+3+0,2*16+5+0,2*16+5+8\
,9*16+1+0,8*16+4+0,0*16+0+8,8*16+7+8,6*16+3+8,6*16+3+0,6*16+5+0,6*16+5+8,0*16+1+0,5*16+3+0,0*16+1+8,5*16+6+8,4*16+3+8,4*16+3+0,4*16+6+0,4*16+6+8\
,1*16+1+0,7*16+5+0,1*16+1+8,7*16+7+8,3*16+2+0,3*16+2+0,3*16+4+0,3*16+4+8,0*16+1+0,1*16+1+0,0*16+1+0,1*16+1+8,2*16+3+0,2*16+3+0,2*16+5+0,2*16+5+8\
,9*16+1+0,8*16+4+0,0*16+0+8,8*16+7+8,6*16+3+8,6*16+3+0,6*16+5+0,6*16+5+8,0*16+1+0,5*16+3+0,0*16+1+8,5*16+6+8,4*16+3+8,4*16+3+0,4*16+6+0,4*16+6+8};

static const char* CPU_OPC = {"\
BRKORAJAMSLONOPORAASLSLOPHPORAASLANCNOPORAASLSLO\
BPLORAJAMSLONOPORAASLSLOCLCORANOPSLONOPORAASLSLO\
JSRANDJAMRLABITANDROLRLAPLPANDROLANCBITANDROLRLA\
BMIANDJAMRLANOPANDROLRLASECANDNOPRLANOPANDROLRLA\
RTIEORJAMSRENOPEORLSRSREPHAEORLSRASRJMPEORLSRSRE\
BVCEORJAMSRENOPEORLSRSRECLIEORNOPSRENOPEORLSRSRE\
RTSADCJAMRRANOPADCRORRRAPLAADCRORARRJMPADCRORRRA\
BVSADCJAMRRANOPADCRORRRASEIADCNOPARRNOPADCRORRRA\
NOPSTANOPSAXSTYSTASTXSAXDEYNOPTXAANESTYSTASTXSAX\
BCCSTAJAMSHASTYSTASTXSAXTYASTATXSSHSSHYSTASHXSHA\
LDYLDALDXLAXLDYLDALDXLAXTAYLDATAXLXALDYLDALDXLAX\
BCSLDAJAMLAXLDYLDALDXLAXCLVLDATSXLAELDYLDALDXLAX\
CPYCMPNOPDCPCPYCMPDECDCPINYCMPDEXSBXCPYCMPDECDCP\
BNECMPJAMDCPNOPCMPDECDCPCLDCMPNOPDCPNOPCMPDECDCP\
CPXSBCNOPISBCPXSBCINCISBINXSBCNOPSBCCPXSBCINCISB\
BEQSBCJAMISBNOPSBCINCISBSEDSBCNOPISBNOPSBCINCISB\
RSTIRQNMI"};

static int Disassemble(MOS6510* p, FIL* file, uint16_t pc, bool line_draw)
{
    static char output[50]; output[0] = 0;
    static char address[7]; address[0] = 0;
    static char memory[16]; memory[0] = 0;
    static char opcode[5]; opcode[0] = 0;
    static char addressing[8]; addressing[0] = 0;

    uint16_t TMP;
    uint16_t OPC;

    uint16_t word;
    uint16_t a;
    char b;

    snprintf(address, 7, "$%4.4X ", pc);			// Adresse Ausgeben

    OPC = MOS6510_Read(p, pc) * 3;					//** Opcodes Ausgeben **//
    snprintf(opcode, 5, "%c%c%c ", CPU_OPC[OPC + 0], CPU_OPC[OPC + 1], CPU_OPC[OPC + 2]);

    TMP = CPU_OPC_INFO[MOS6510_Read(p, pc)];		//** Memory und Adressierung Ausgeben **//
    TMP = TMP >> 4;
    TMP = TMP & 15;

    switch(TMP)
    {
    case 0:     //** Implizit **//
        snprintf(memory, 16, "$%2.2X          ", MOS6510_Read(p, pc));
        addressing[0] = 0;
        pc++;
        break;

    case 1:		//** Unmittelbar **//
        snprintf(memory, 16, "$%2.2X $%2.2X      ", MOS6510_Read(p, pc), MOS6510_Read(p, pc + 1));
        snprintf(addressing, 8, "#$%2.2X", MOS6510_Read(p, pc + 1));
        pc += 2;
        break;

    case 2:		//** Absolut **//
        snprintf(memory, 16, "$%2.2X $%2.2X $%2.2X  ", MOS6510_Read(p, pc), MOS6510_Read(p, pc + 1), MOS6510_Read(p, pc + 2));
        word = MOS6510_Read(p, pc + 1);
        word|=MOS6510_Read(p, pc + 2)<<8;
        snprintf(addressing, 8, "$%4.4X", word);
        pc += 3;
        break;

    case 3:		//** Zerropage **//
        snprintf(memory, 16, "$%2.2X $%2.2X      ", MOS6510_Read(p, pc), MOS6510_Read(p, pc + 1));
        snprintf(addressing, 8, "$%2.2X", MOS6510_Read(p, pc + 1));
        pc += 2;
        break;

    case 4:		//** Absolut X Indexziert **//
        snprintf(memory, 16, "$%2.2X $%2.2X $%2.2X  ", MOS6510_Read(p, pc), MOS6510_Read(p, pc + 1), MOS6510_Read(p, pc + 2));
        word=MOS6510_Read(p, pc + 1);
        word|=MOS6510_Read(p, pc + 2)<<8;
        snprintf(addressing, 8, "$%4.4X,X", word);
        pc += 3;
        break;

    case 5:		//** Absolut Y Indexziert **//
        snprintf(memory, 16, "$%2.2X $%2.2X $%2.2X  ", MOS6510_Read(p, pc), MOS6510_Read(p, pc + 1), MOS6510_Read(p, pc + 2));
        word=MOS6510_Read(p, pc + 1);
        word|=MOS6510_Read(p, pc + 2)<<8;
        snprintf(addressing, 8, "$%4.4X,Y", word);
        pc += 3;
        break;

    case 6:		//** Zerropage X Indexziert **//
        snprintf(memory, 16, "$%2.2X $%2.2X      ", MOS6510_Read(p, pc), MOS6510_Read(p, pc + 1));
        snprintf(addressing, 8, "$%2.2X,X", MOS6510_Read(p, pc + 1));
        pc += 2;
        break;

    case 7:		//** Indirekt X Indiziert **//
        snprintf(memory, 16, "$%2.2X $%2.2X      ", MOS6510_Read(p, pc), MOS6510_Read(p, pc + 1));
        snprintf(addressing, 8, "($%2.2X,X)", MOS6510_Read(p, pc + 1));
        pc += 2;
        break;

    case 8:		//** Indirekt Y Indiziert **//
        snprintf(memory, 16, "$%2.2X $%2.2X      ", MOS6510_Read(p, pc), MOS6510_Read(p, pc + 1));
        snprintf(addressing, 8, "($%2.2X),Y", MOS6510_Read(p, pc + 1));
        pc += 2;
        break;

    case 9:
        snprintf(memory, 16, "$%2.2X $%2.2X      ", MOS6510_Read(p, pc), MOS6510_Read(p, pc + 1));
        b = MOS6510_Read(p, pc + 1);
        a = (pc + 2) + b;
        snprintf(addressing, 8, "$%4.4X", a);
        pc += 2;
        break;

    case 10:	//** Indirekt **//
        snprintf(memory, 16, "$%2.2X $%2.2X $%2.2X  ", MOS6510_Read(p, pc), MOS6510_Read(p, pc + 1), MOS6510_Read(p, pc + 2));
        word = MOS6510_Read(p, pc + 1);
        word |= MOS6510_Read(p, pc + 2) << 8;
        snprintf(addressing, 8, "($%4.4X)", word);
        pc += 3;
        break;

    case 11:									//** Zerropage Y Indexziert **//
        snprintf(memory, 16, "$%2.2X $%2.2X      ", MOS6510_Read(p, pc), MOS6510_Read(p, pc + 1));
        snprintf(addressing, 8, "$%2.2X,Y", MOS6510_Read(p, pc + 1));
        pc += 2;
        break;
    }

    snprintf(output, 50, "%s%s%s%s", address, memory, opcode, addressing);
    fprintf(file, "%s\n", output);

    OPC /= 3;
    if(((OPC == 0x40) || (OPC == 0x60) || (OPC == 0x4C)) && (line_draw == true))
    {
        fprintf(file, "------------------------------\n");
    }

    return pc;
}
