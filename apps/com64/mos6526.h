//////////////////////////////////////////////////
//                                              //
// Emu64                                        //
// von Thorsten Kattanek                        //
//                                              //
// #file: mos6526_class.h                       //
//                                              //
// Dieser Sourcecode ist Copyright geschützt!   //
// Geistiges Eigentum von Th.Kattanek           //
//                                              //
// Letzte Änderung am 22.09.2019                //
// www.emu64.de                                 //
//                                              //
//////////////////////////////////////////////////

#ifndef MOS_6526_H
#define MOS_6526_H

#include "mos6526_port.h"
#include "structs.h"

#define ClockCounterPAL 98438
#define ClockCounterNTSC 118126
#define Byte2BCD(byte)  (((((byte) / 10) << 4) + ((byte) % 10)) & 0xFF)
#define BCD2Byte(bcd)   (((10*(((bcd) & 0xf0) >> 4)) + ((bcd) & 0xf)) & 0xFF)

typedef struct MOS6526
{
	// Timer states
	enum 
	{
            STOP,
            WAIT_THEN_COUNT,
            LOAD_THEN_STOP,
            LOAD_THEN_COUNT,
            LOAD_THEN_WAIT_THEN_COUNT,
            COUNT,
            COUNT_THEN_STOP,
	};

    bool *reset_wire;
    bool reset_wire_old;
    func_vi_t CpuTriggerInterrupt;
    func_vi_t CpuClearInterrupt;
    func_vv_t VicTriggerLP;
    func_vv_t ChangePOTSwitch;

    PORT        *pa;
    PORT        *pb;
    uint8_t     pa_latch;
    uint8_t     pb_latch;
    uint8_t     sdr;

    uint8_t     *c64_iec_wire;
    uint8_t     *floppy_iec_wire;
    bool        *flag_pin;
    bool        flag_pin_old;
    bool        cnt_pin;

    uint8_t     cia_nr;
    uint8_t     io[16];
    uint8_t     interupt_mask;
    uint8_t     cra, crb, icr;
    uint8_t     new_cra, new_crb;
    bool        writing_new_cra, writing_new_crb;
    bool        timer_a_cnt_phi2;
    bool        timer_b_cnt_phi2;
    bool        timer_b_cnt_timer_a;
    bool        timer_b_cnt_cnt_pin;
    uint8_t     timer_a_status;
    uint8_t     timer_b_status;
    uint16_t    timer_a;
    uint16_t    timer_a_latch;
    uint16_t    timer_b;
    uint16_t    timer_b_latch;

    uint32_t    clock_counter_latch;
    uint32_t    clock_counter;
    bool        clock_is_latched;
    bool        clock_is_stopped;
    uint8_t     clock_latch[4];
    uint8_t     clock_alarm[4];

    uint8_t     ddr_a;
    uint8_t     ddr_b;
    bool        enable_pb6;
    bool        enable_pb7;
    uint8_t     pb6_mode;
    uint8_t     pb7_mode;
    uint8_t     pb_6;
    uint8_t     pb_7;
    uint8_t     prev_lp;
} MOS6526;

inline static void MOS6526_MOS6526(MOS6526* p, uint8_t cia_nr)
{
    p->cia_nr = cia_nr;
    p->cnt_pin = true; // Default True da am Flag nichts angeschlossen ist !
}


#endif // MOS_6526_H
