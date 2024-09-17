#include "mos6502.h"
#include "micro_code_tbl_6502.h"
#include "mmu.h"

static void MOS6502_MOS6502(MOS6502* p, MMU* mmu)
{
    p->ReadProcTbl = mmu->CPUReadProcTbl;
    p->WriteProcTbl = mmu->CPUWriteProcTbl;
    p->MCT = ((unsigned char*)MicroCodeTable6502 + (0x100*MCTItemSize));
    p->SR = 32;
}

static void MOS6502_Reset(MOS6502* p)
{
    p->SR=0x24;
    p->SP=0;
    p->PC=0;
    p->AC=0;
    p->XR=0;
    p->YR=0;

    p->MCT = ((unsigned char*)MicroCodeTable6502 + (0x100*MCTItemSize));
    p->JAMFlag = false;
    p->AktOpcodePC = p->PC;
    p->PC++;
}

static void MOS6502_TriggerInterrupt(MOS6502* p, int typ)
{
    switch (typ)
    {
    case VIA1_IRQ:
        p->irq_state |= 0x01;
        break;
    case VIA2_IRQ:
        p->irq_state |= 0x02;
        break;
    }
}

static void MOS6502_ClearInterrupt(MOS6502* p, int typ)
{
    switch (typ)
    {
    case VIA1_IRQ:
        p->irq_state &= ~0x01;
        break;
    case VIA2_IRQ:
        p->irq_state &= ~0x02;
        break;
    }
}

static void MOS6502_SetRegister(MOS6502* p, REG_STRUCT *reg)
{
    if(reg == 0) return;

    unsigned char mask = reg->reg_mask;
    if((mask&1) == 1)
    {
        p->PC = reg->pc;
        p->MCT = ((unsigned char*)MicroCodeTable6502 + 6);
    }
    mask>>=1;
    if((mask&1) == 1) p->AC = (unsigned char)reg->ac;
    mask>>=1;
    if((mask&1) == 1) p->XR = (unsigned char)reg->xr;
    mask>>=1;
    if((mask&1) == 1) p->YR = (unsigned char)reg->yr;
    mask>>=1;
    if((mask&1) == 1) p->SP = (unsigned char)reg->sp;
    mask>>=1;
    if((mask&1) == 1) p->SR = (unsigned char)reg->sr | 32;
}

static void MOS6502_GetRegister(MOS6502* p, REG_STRUCT *reg)
{
    if(reg == 0) return;

    unsigned char mask = reg->reg_mask;
    if((mask&1) == 1) reg->pc = p->PC;
    mask>>=1;
    if((mask&1) == 1) reg->ac = p->AC;
    mask>>=1;
    if((mask&1) == 1) reg->xr = p->XR;
    mask>>=1;
    if((mask&1) == 1) reg->yr = p->YR;
    mask>>=1;
    if((mask&1) == 1) reg->sp = p->SP;
    mask>>=1;
    if((mask&1) == 1) reg->sr = p->SR;
    mask>>=1;
    if((mask&1) == 1)
    {
        reg->irq = MOS6502_Read(p, 0xFFFE);
        reg->irq |= MOS6502_Read(p, 0xFFFF) << 8;
    }
    mask>>=1;
    if((mask&1) == 1)
    {
        reg->nmi = MOS6502_Read(p, 0xFFFA);
        reg->nmi |= MOS6502_Read(p, 0xFFFB) << 8;
    }
    reg->_0314 = MOS6502_Read(p, 0x314);
    reg->_0314 |= MOS6502_Read(p, 0x315) << 8;
    reg->_0318 = MOS6502_Read(p, 0x318);
    reg->_0318 |= MOS6502_Read(p, 0x319) << 8;
}
