#pragma once

#include "8253.h"
///#include "AySound.h"
#include "fd1793.h"
///#include "wav.h"
#include "memory.h"

#include "serialize.h"

class IO {
private:
    Memory & kvaz;
    I8253 & timer;
    FD1793 & fdc;
    WavPlayer & tape_player;

    uint8_t CW, PA, PB, PC, PIA1_last;
    uint8_t CW2, PA2, PB2, PC2;
    uint8_t joy_0e, joy_0f;

public:
    int outport;
    int outbyte;
    int palettebyte;
public:
    typedef void (*vi_ptr)(int);
    typedef void (*vb_ptr)(bool);
    vi_ptr onborderchange;
    vb_ptr onmodechange;
    vb_ptr onruslat;
    typedef uint32_t (*u32u8u8u8_ptr)(uint8_t,uint8_t,uint8_t);
    u32u8u8u8_ptr rgb2pixelformat;

    typedef int (*iu32u8_ptr)(uint32_t,uint8_t);
    iu32u8_ptr onread;
    typedef void (*vu32u8_ptr)(uint32_t,uint8_t);
    vu32u8_ptr onwrite;

public:
    IO(Memory & _memory, I8253 & _timer, FD1793 & _fdc, 
            WavPlayer & _tape_player) 
        : kvaz(_memory), timer(_timer), fdc(_fdc),
        tape_player(_tape_player),
        CW(0x08), PA(0xff), PB(0xff), PC(0xff), CW2(0), PA2(0xff), PB2(0xff), PC2(0xff)
    {
        outport = outbyte = palettebyte = -1;
        joy_0e = joy_0f = 0xff;
    }

    int input(int port)
    {
        int result = 0xff;
        
        switch(port) {
            case 0x00:
                result = 0xff;
                break;
            case 0x01:
                {
                /* PC.low input ? */
                auto pclow = (this->CW & 0x01) ? 0x0b : (this->PC & 0x0f);
                /* PC.high input ? */
                auto pcupp = (this->CW & 0x08) ? ((this->tape_player.sample() << 4) | keyboard::io_state.pc) : (this->PC & 0xf0);
                result = pclow | pcupp;
                }
                break;
            case 0x02:
                if ((this->CW & 0x02) != 0) {
                    keyboard::io_read_rows();
                    result = keyboard::io_state.rows;
                    ///*if (this->PA == 0xef)*/ printf("%02x %02x\n", this->PA, result);
                } else {
                    result = this->PB;       // output
                }
                break;
            case 0x03:
                if ((this->CW & 0x10) == 0) { 
                    result = this->PA;       // output
                } else {
                    result = 0xff;          // input
                }
                break;

            case 0x04:
                result = this->CW2;
                break;
            case 0x05:
                result = this->PC2;
                break;
            case 0x06:
                result = this->PB2;
                break;
            case 0x07:
                result = this->PA2;
                break;

                // Timer
            case 0x08:
            case 0x09:
            case 0x0a:
            case 0x0b:
                return this->timer.read(~(port & 3));

                // Joystick "C"
            case 0x0e:
                return this->joy_0e;
            case 0x0f:
                return this->joy_0f;

            case 0x14:
                result = AySound::getRegisterData();
                break;
            case 0x15:
                result = 0xff;
                break;

            case 0x18: // fdc data
                result = this->fdc.read(3);
                break;
            case 0x19: // fdc sector
                result = this->fdc.read(2);
                break;
            case 0x1a: // fdc track
                result = this->fdc.read(1);
                break;
            case 0x1b: // fdc status
                result = this->fdc.read(0);
                break;
            case 0x1c: // fdc control - readonly
                //result = this->fdc.read(4);
                break;
            default:
                break;
        }

        if (this->onread) {
            int hookresult = this->onread((uint32_t)port, (uint8_t)result);
            if (hookresult != -1) {
                result = hookresult;
            }
        }

        return result;
    }

    void output(int port, int w8) {
        if (this->onwrite) {
            this->onwrite((uint32_t)port, (uint8_t)w8);
        }
        this->outport = port;
        this->outbyte = w8;

        //if (port == 0x02) {
        //    this->onmodechange((w8 & 0x10) != 0);
        //}
        #if 0
        /* debug print from guest */
        switch (port) {
            case 0x77:  
                this->str1 += w8.toString(16) + " ";
                break;
            case 0x79:
                if (w8 != 0) {
                    this->str1 += String.fromCharCode(w8);
                } else {
                    console.log(this->str1);
                    this->str1 = "";
                }
        }
        #endif
    }
    
    void realoutput(int port, int w8) {
        //printf("realoutput: %d <- %02x\n", port, w8);
        bool ruslat;
        switch (port) {
            // PIA 
            case 0x00:
                this->PIA1_last = w8;
                ruslat = this->PC & 8;
                if ((w8 & 0x80) == 0) {
                    // port C BSR: 
                    //   bit 0: 1 = set, 0 = reset
                    //   bit 1-3: bit number
                    int bit = (w8 >> 1) & 7;
                    if ((w8 & 1) == 1) {
                        this->PC |= 1 << bit;
                    } else {
                        this->PC &= ~(1 << bit);
                    }
                    //this->ontapeoutchange(this->PC & 1);
                } else {
                    this->CW = w8;
                    this->realoutput(1, 0);
                    this->realoutput(2, 0);
                    this->realoutput(3, 0);
                }
                if ((this->PC & 8) != ruslat) {
                    keyboard::io_out_ruslat(this->PC);
                    if (this->onruslat) this->onruslat((this->PC & 8) == 0);
                }
                // if (debug) {
                //     console.log("output commit cw = ", this->CW.toString(16));
                // }
                break;
            case 0x01:
                this->PIA1_last = w8;
                ruslat = this->PC & 8;
                this->PC = w8;
                //this->ontapeoutchange(this->PC & 1);
                if ((this->PC & 8) != ruslat) {
                    keyboard::io_out_ruslat(this->PC);
                    if (this->onruslat) this->onruslat((this->PC & 8) == 0);
                }
                break;
            case 0x02:
                this->PIA1_last = w8;
                this->PB = w8;
                this->onborderchange(this->PB & 0x0f);
                this->onmodechange((this->PB & 0x10) != 0);
                break;
            case 0x03:
                this->PIA1_last = w8;
                this->PA = w8;
                keyboard::io_select_columns(w8);
                break;
                // PPI2
            case 0x04:
                this->CW2 = w8;
                break;
            case 0x05:
                this->PC2 = w8;
                break;
            case 0x06:
                this->PB2 = w8;
                break;
            case 0x07:
                this->PA2 = w8;
                break;

                // Timer
            case 0x08:
            case 0x09:
            case 0x0a:
            case 0x0b:
                //printf("timer.write %x=%x\n", port, w8);
                this->timer.write((~port & 3), w8);
                //printf("timer.write done\n");
                break;

            case 0x0c:
            case 0x0d:
            case 0x0e:
            case 0x0f:
                this->palettebyte = w8;
                break;
            case 0x10:
                // kvas 
                this->kvaz.control_write(w8);
                break;
            case 0x14:
                // in esp_filler we catch up generation before writing
                AySound::setRegisterData(w8);
                break;
            case 0x15:
                AySound::selectRegister(w8 & 0xf);
                //printf("ay.write %x=%x\n", port, w8);
                //this->ay.write(port & 1, w8);
                //printf("ay.write done\n");
                break;

            case 0x18: // fdc data
                this->fdc.write(3, w8);
                break;
            case 0x19: // fdc sector
                this->fdc.write(2, w8);
                break;
            case 0x1a: // fdc track
                this->fdc.write(1, w8);
                break;
            case 0x1b: // fdc command
                this->fdc.write(0, w8);
                break;
            case 0x1c: // fdc control
                this->fdc.write(4, w8);
                break;
            default:
                break;
        }
    }

    void commit() 
    {
        if (this->outport != -1) {
            //if (this->outport < 16)  printf("io::commit: %02x = %02x\n", this->outport, this->outbyte);
            this->realoutput(this->outport, this->outbyte);
            this->outport = this->outbyte = -1;
        }
    }

    int BorderIndex() const 
    {
        return this->PB & 0x0f;
    }

    int ScrollStart() const 
    {
        return this->PA;
    }

    bool Mode512() const 
    {
        return (this->PB & 0x10) != 0;
    }

    int TapeOut() const 
    {
        return this->PC & 1;
    }

    int Covox() const 
    {
        return this->PA2;
    }

    uint8_t pa() const 
    {
        return this->PA;
    }

    uint8_t pb() const
    {
        return this->PB;
    }

    uint8_t pc() const 
    {
        return this->PC;
    }

    // same as joystick "C", active 0
    void set_joysticks(int j0e, int j0f)
    {
        this->joy_0e = j0e;
        this->joy_0f = j0f;

        // USPID = PA2
        uint8_t inv = ~j0e;
        this->PA2 = ((inv & 0x40) >> 3) | // button
            ((inv & 0x02) << 3) | // left
            ((inv & 0x08) << 2) | // down
            ((inv & 0x01) << 6) | // right
            ((inv & 0x04) << 5);

        // PU0
        this->PB2 = j0e;              
    }

    void serialize(vector<uint8_t> & to)
    {
        vector<uint8_t> tmp;
        tmp.push_back(CW);
        tmp.push_back(PA);
        tmp.push_back(PB);
        tmp.push_back(PC);
        tmp.push_back(PIA1_last);
        tmp.push_back(CW2);
        tmp.push_back(PA2);
        tmp.push_back(PB2);
        tmp.push_back(PC2);

        SerializeChunk::insert_chunk(to, SerializeChunk::IO, tmp);
    }

    void deserialize(vector<uint8_t>::iterator it, uint32_t size)
    {
        CW = *it++;
        PA = *it++;
        PB = *it++;
        PC = *it++;
        PIA1_last = *it++;
        CW2 = *it++;
        PA2 = *it++;
        PB2 = *it++;
        PC2 = *it++;

        if (this->onmodechange) this->onmodechange((this->PB & 0x10) != 0);
        if (this->onborderchange) this->onborderchange(this->PB & 0x0f);
        if (this->onruslat) this->onruslat((this->PC & 8) == 0);
    }
};
