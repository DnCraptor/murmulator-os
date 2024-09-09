#pragma once

///#include "esp_attr.h"
///#include "params.h"

class CounterUnit
{
    friend class TestOfCounterUnit;

    int latch_value;
    int write_state;
    int latch_mode;
    int mode_int;

    uint8_t write_lsb;
    uint8_t write_msb;
public:
    union {
        uint32_t flags;
        struct {
            bool armed:1;
            bool load:1;
            bool enabled:1;
            bool bcd:1;
        };
    };

    uint8_t out;
    uint16_t loadvalue;
    int value;

public:
    CounterUnit();
    void reset();

    void SetMode(int new_mode, int new_latch_mode, int new_bcd_mode);
    void Latch(uint8_t w8);
    void mode0(int nclocks);
    void mode1(int nclocks);
    void mode2(int nclocks);
    void mode3(int nclocks);
    void mode4(int nclocks);
    void mode5(int nclocks);

    void count_clocks(int nclocks);
    void write_value(uint8_t w8);
    int read_value();
};

typedef int16_t audio_sample_t;

class I8253
{
private:
    CounterUnit counters[3];
    uint8_t control_word;
    //int clock_carry;
    int16_t counted_carry;
    int16_t accu_carry;


public:
    audio_sample_t* audio_buf; // pointer to external sound buf
    uint8_t beeper;
    uint8_t tapein;
    uint8_t covox;

    typedef int (*count_tape_t)(int);
    count_tape_t count_tape;
///    std::function<int(int)> count_tape;
public:
    I8253();
    void write_cw(uint8_t w8);
    void write(int addr, uint8_t w8);
    int read(int addr);
    void count_clocks(int nclocks);
    void gen_sound(int nclocks);
    void reset();
};

