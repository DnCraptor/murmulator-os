#ifndef V06X_SERIALIZE_H
#define V06X_SERIALIZE_H

///#include <cstdint>

class SerializeChunk {
public:
    typedef vector<uint8_t> stype_t;

    enum id : uint32_t {
        MEMORY = 0xfecacaca,
        IO = 0xfeca1010,
        CPU = 0xfeca0000,
        BOARD = 0xfecababe,
        DEBUG = 0xfefadada,
    };

    static void insert_chunk(stype_t &to, id chunk_id, stype_t &chunk)
    {
        /*** TODO:
        uint8_t * pchunk = reinterpret_cast<uint8_t *>(&chunk_id);
        to.insert(to.end(), pchunk, pchunk + 4);

        uint32_t size = chunk.size();
        uint8_t * psize = reinterpret_cast<uint8_t *>(&size);
        to.insert(to.end(), psize, psize + 4);

        to.insert(to.end(), chunk.begin(), chunk.end());
         */
    }

    static stype_t::iterator take_chunk(stype_t::iterator it, id & signature, uint32_t & size)
    {
        /*** TODO:
        uint32_t sig;
        copy(it, it + 4, reinterpret_cast<uint8_t *>(&sig));
        signature = static_cast<id>(sig);
        it += 4;

        uint32_t size_;
        copy(it, it + 4, reinterpret_cast<uint8_t *>(&size_));
        size = size_;
        return it + 4;
         */
        return it;
    }
};

#endif
