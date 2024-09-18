//////////////////////////////////////////////////
//                                              //
// Emu64                                        //
// von Thorsten Kattanek                        //
//                                              //
// #file: mos6526_port.h                        //
//                                              //
// Dieser Sourcecode ist Copyright geschützt!   //
// Geistiges Eigentum von Th.Kattanek           //
//                                              //
// Letzte Änderung am 15.06.2019                //
// www.emu64.de                                 //
//                                              //
//////////////////////////////////////////////////

#ifndef MOS_6526_PORT_H
#define MOS_6526_PORT_H

#include <stdint.h>

typedef struct PORT
{
    uint8_t input_bits;
    uint8_t output_bits;
} PORT;

inline static uint8_t *PORT_GetOutputBitsPointer(PORT* p)
{
    return &p->output_bits;
}

inline static uint8_t *PORT_GetInputBitsPointer(PORT* p)
{
    return &p->input_bits;
}

inline static unsigned char PORT_GetInput(PORT* p)
{
    return ~p->input_bits;
}

inline static void PORT_SetInput(PORT* p, uint8_t value)
{
    p->input_bits = ~value;
}

inline static uint8_t PORT_GetOutput(PORT* p)
{
    return ~p->output_bits;
}

inline static void PORT_SetOutput(PORT* p, uint8_t value)
{
    p->output_bits = ~value;
}

#endif // MOS_6526_PORT_H
