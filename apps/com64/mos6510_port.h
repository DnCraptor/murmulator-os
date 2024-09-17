#ifndef MOS_6510_PORT_H
#define MOS_6510_PORT_H

typedef struct MOS6510_PORT
{
    // Wert zum Prozessor Port schreiben
    unsigned char DIR;
    unsigned char DATA;

    // Wert vom Prozessor Port lesen
    unsigned char DIR_READ;
    unsigned char DATA_READ;

    // Status der Prozessor Port Pins
    unsigned char DATA_OUT;

    /* cycle that should invalidate the unused bits of the data port. */
    unsigned long DATA_SET_CLK_BIT6;
    unsigned long DATA_SET_CLK_BIT7;

    /* indicates if the unused bits of the data port are still
    valid or should be read as 0, 1 = unused bits valid,
    0 = unused bits should be 0 */
    unsigned char DATA_SET_BIT6;
    unsigned char DATA_SET_BIT7;

    /* indicated if the unused bits are in the process of falling off. */
    unsigned char DATA_FALLOFF_BIT6;
    unsigned char DATA_FALLOFF_BIT7;

    bool TAPE_SENSE;
    bool DATASETTE_MOTOR;
} MOS6510_PORT;

inline static void MOS6510_PORT_Reset(MOS6510_PORT* p)
{
	p->DATA = 0x3f;
	p->DATA_OUT = 0x3f;
	p->DATA_READ = 0x3f;
	p->DIR = 0;
	p->DIR_READ = 0;
	p->DATA_SET_BIT6 = 0;
	p->DATA_SET_BIT7 = 0;
	p->DATA_FALLOFF_BIT6 = 0;
	p->DATA_FALLOFF_BIT7 = 0;
    p->TAPE_SENSE = false;
}

inline static void MOS6510_PORT_MOS6510_PORT(MOS6510_PORT* p)
{
	MOS6510_PORT_Reset(p);
}

inline static void MOS6510_PORT_ConfigChanged(MOS6510_PORT* p, int tape_sense, int caps_sense,unsigned char pullup)
{
	p->TAPE_SENSE = !!(tape_sense);
	/* Tape Motor Status.  */
	static unsigned char OLD_DATA_OUT = 0xFF;
	/* Tape Write Line Status */
	static unsigned char OLD_WRITE_BIT = 0xFF;

	p->DATA_OUT = (p->DATA_OUT & ~p->DIR) | (p->DATA & p->DIR);
	p->DATA_READ = (p->DATA | ~p->DIR) & (p->DATA_OUT | pullup);

	if ((pullup & 0x40) && !caps_sense) p->DATA_READ &= 0xBF;
	if (!(p->DIR & 0x20)) p->DATA_READ &= 0xDF;
	if (tape_sense && !(p->DIR & 0x10)) p->DATA_READ &= 0xEF;
	if (((p->DIR & p->DATA) & 0x20) != OLD_DATA_OUT)
	{
		OLD_DATA_OUT = (p->DIR & p->DATA) & 0x20;
		p->DATASETTE_MOTOR = !OLD_DATA_OUT;
	}

	if (((~p->DIR | p->DATA) & 0x8) != OLD_WRITE_BIT)
	{
		OLD_WRITE_BIT = (~p->DIR | p->DATA) & 0x8;
		//datasette_toggle_write_bit((~pport.dir | pport.data) & 0x8);
	}
	p->DIR_READ = p->DIR;
}

#endif
