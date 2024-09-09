#pragma once


class Memory {
private:
///    uint8_t bytes[TOTAL_MEMORY];
    uint8_t mode_reg;
    uint32_t page_map;
    uint32_t page_stack;

    vector<uint8_t> bootbytes;

    static uint32_t tobank(uint32_t a);

public:
    uint32_t bigram_select(uint32_t addr, bool stackrq) const;
    uint8_t get_byte(uint32_t addr, bool stackrq) const;
    /* virtual addr, physical addr, stackrq, value */
///    std::function<void(uint32_t,uint32_t,bool,uint8_t)> onwrite;
///    std::function<void(uint32_t,uint32_t,bool,uint8_t)> onread;

///    std::function<void(const uint32_t, const uint8_t, const bool)> debug_onread;
///    std::function<void(const uint32_t, const uint8_t)> debug_onwrite;

public:
    Memory();
    void control_write(uint8_t w8);

    inline bool mode_stack() const { return (this->mode_reg & 0x10) != 0; }
    inline uint8_t mode_map() const { return  this->mode_reg & 0xe0; }

    uint8_t read(uint32_t addr, bool stackrq, const bool _is_opcode = false) const;
    void write(uint32_t addr, uint8_t w8, bool stackrq);
    void init_from_vector(const vector<uint8_t> & from, uint32_t start_addr);
    void attach_boot(vector<uint8_t> boot);
    void detach_boot();
    uint8_t * buffer();
    size_t buffer_size() const { return TOTAL_MEMORY; } /// TODO: ensure
    #if 0
    heatmap_t& get_heatmap() { return heatmap; }
    void cool_off_heatmap();
    #endif
    void export_bytes(uint8_t * dst, uint32_t addr, uint32_t size) const;

    void serialize(vector<uint8_t> & to);
    void deserialize(vector<uint8_t>::iterator from, uint32_t size);
    
    bool get_mode_stack() const;
    uint8_t get_mode_map() const;
    uint32_t get_page_map() const;
    uint32_t get_page_stack() const;
};
