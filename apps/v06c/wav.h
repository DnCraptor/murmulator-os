#pragma once

class Wav
{
private:
    typedef uint8_t sample_t;
    vector<sample_t> Data;
public:
    uint16_t NumChannels;
    uint32_t SampleRate;
    uint32_t ByteRate;
    uint16_t BlockAlign;
    uint16_t BitsPerSample;

    typedef struct onloaded_t {
        void* that;
        typedef void (*onloaded_fn_t)(void*);
        onloaded_fn_t fn;
    } onloaded_t;
    onloaded_t onloaded;
private:
/**
    template <typename valtype> 
    typename enable_if<!is_same<valtype,string>::value, valtype>::type
        token(const vector<uint8_t> & raw, size_t & offset)
    {
        if (offset + sizeof(valtype) >= raw.size()) {
            throw std::invalid_argument("stream too short");
        }

        valtype result = *(valtype *)&raw[offset];
        offset += sizeof(valtype);
        return result;
    }

    template <typename valtype>
    typename enable_if<is_same<valtype,string>::value, valtype>::type
        token(const vector<uint8_t> & raw, size_t & offset)
    {
        if (offset + 4 >= raw.size()) {
            throw std::invalid_argument("stream too short");
        }

        string result((const char *) &raw[offset], 4); 
        offset += 4;
        return result;
    }

    template <typename srctype, 
                typename enable_if<is_same<srctype, uint8_t>::value, srctype>::type = 0
             >
    void merge_stereo(const void * _src, size_t count)
    {
        const srctype * src = (srctype *) _src;
        for (size_t i = 0, o = 0; i < count;) {
            int y = src[i++];
            if (this->NumChannels == 2) {
                y = (y + src[i++]) / 2;
            }
            this->Data[o++] = y;
        }
    }

    template <typename srctype, 
                typename enable_if<!is_same<srctype, uint8_t>::value, srctype>::type = 0
             >
    void merge_stereo(const void * _src, size_t count)
    {
        const srctype * src = (srctype *) _src;
        
        for (size_t i = 0, o = 0; i < count;) {
            int y = src[i++];
            if (this->NumChannels == 2) {
                y += src[i++];
                y /= 2;
            }
            y /= 256;
            y += 128;
            this->Data[o++] = y;
        }
    }

    // sptr is modified!
    bool parse_header(const vector<uint8_t> & raw, size_t & sptr)
    {
        string chunkid = token<string>(raw, sptr);

        if (chunkid != "RIFF") {
            printf("Not a RIFF file\n");
            return false;
        }

        uint32_t globsize = token<uint32_t>(raw, sptr); (void)globsize;

        string wave = token<string>(raw, sptr);
        if (wave != "WAVE") {
            printf("No WAVE format signature\n");
            return false;
        }

        string fmtid = token<string>(raw, sptr);
        if (fmtid != "fmt ") {
            printf("No fmt subchunk\n");
            return false;
        }

        uint32_t subchunksize = token<uint32_t>(raw, sptr);
        size_t nextchunk = sptr + subchunksize;

        uint16_t audioformat = token<uint16_t>(raw, sptr);
        if (audioformat != 1) {
            printf("Audioformat must be type 1, PCM\n");
            return false;
        }

        this->NumChannels = token<uint16_t>(raw, sptr);
        this->SampleRate =  token<uint32_t>(raw, sptr);
        this->ByteRate =    token<uint32_t>(raw, sptr);
        this->BlockAlign =  token<uint16_t>(raw, sptr);
        this->BitsPerSample=token<uint16_t>(raw, sptr);

        printf("WAV file: Channels: %d Sample rate: %lu Byte rate: %lu "
                "Block align: %u Bits per sample: %u\n",
                this->NumChannels, this->SampleRate, this->ByteRate, 
                this->BlockAlign, this->BitsPerSample);

        sptr = nextchunk;
        return true;
    }
*/
public:
/*
    bool set_bytes(const vector<uint8_t> & raw)
    {
        size_t sptr = 0;

        try {
            bool header_ok = parse_header(raw, sptr);
            if (!header_ok) return false;
        }
        catch (...) {
            return false;
        }

        size_t nextchunk = sptr;
        string chunk2id;
        uint32_t chunk2sz;
        for (;;) {
            sptr = nextchunk;
            try {
                chunk2id = token<string>(raw, sptr);
                chunk2sz = token<uint32_t>(raw, sptr);
            }
            catch (...) {
                return false;
            }

            nextchunk = sptr + chunk2sz;
            // check that sptr + chunk2sz is within raw vector bounds
            if (sptr + chunk2sz > raw.size()) {
                uint32_t orig = chunk2sz;
                chunk2sz = raw.size() - sptr;
                nextchunk = raw.size();
                printf("Warning: reported chunk size: %lu, "
                        "available: %lu bytes\n", orig, chunk2sz);
            }

            if (chunk2id == "data") {
                switch (this->BitsPerSample) {
                    case 8:
                        this->Data.resize(chunk2sz / this->NumChannels);
                        this->merge_stereo<uint8_t>(&raw[sptr], chunk2sz);
                        break;
                    case 16:
                        this->Data.resize(chunk2sz / 2 / this->NumChannels);
                        this->merge_stereo<int16_t>(&raw[sptr], chunk2sz / 2);
                        break;
                    case 32:
                        this->Data.resize(chunk2sz / 4 / this->NumChannels);
                        this->merge_stereo<int32_t>(&raw[sptr], chunk2sz / 4);
                        break;
                    default:
                        printf("Unsupported bits per sample: %d\n", 
                                this->BitsPerSample);
                        return false;
                        break;
                }
                if (this->onloaded) {
                    this->onloaded();
                }
                return true;
            }
        }
        return false;
    }
*/
    void set_raw_bytes(const vector<uint8_t> & bytes)
    {
        printf("wav: set_raw_bytes: %d samples\n", bytes.size());
        this->Data.resize(bytes.size());
///        this->merge_stereo<uint8_t>(bytes.data(), bytes.size());
        if (this->onloaded.fn) {
            this->onloaded.fn(this->onloaded.that);
        }
    }

    int sample_at(size_t pos) const 
    {
        return (int)this->Data[pos] - 127;
    }

    size_t size() const 
    {
        return this->Data.size();
    }
};

class WavPlayer
{
private:
    Wav & wav;
    size_t playhead;
    int ratio;
    int frac;
    bool loaded;


public:
    typedef void (*finished_t)(int);
    struct {
        finished_t finished;
    } hooks;

public:
    static void on_loaded(WavPlayer* that) {
        that->loaded = true;
        that->rewind();
    }
    WavPlayer(Wav & _wav) : wav(_wav)
    {
        wav.onloaded = { this, (Wav::onloaded_t::onloaded_fn_t)on_loaded };
    }

    void init()
    {
        this->ratio = 59904 * 50 / wav.SampleRate;
        this->frac = 0;
    }

    void rewind()
    {
        this->playhead = 0;
        this->ratio = 0;
        this->frac = 0;
    }

    void advance(int instruction_time)
    {
        if (this->loaded) {
            if(this->playhead < this->wav.size()) {
                if (this->ratio == 0) {
                    this->init();
                }
                this->frac += instruction_time;
                while (this->frac > this->ratio) {
                    this->frac -= this->ratio;
                    ++this->playhead;
                }
            }
            else {
                this->loaded = false;

                printf("wavplayer finished\n");
                if (hooks.finished != nullptr) {
                    printf("wavplayer invoking finished hook\n");
                    hooks.finished(0);
                }
            }
        }
    }

    int sample() const
    {
        if (this->loaded && playhead < this->wav.size()) {
            return wav.sample_at(playhead) > 0 ? 1 : 0;
        }
        return 0;
    }
};

class WavRecorder 
{
private:
///    ofstream file;
    vector<int16_t> buffer;
    static const size_t buffer_size = 65536;
    size_t offset = 0;
    size_t length_pos;
    uint32_t data_size;

public:
    WavRecorder()
    {
    }

    void init(const string & path)
    {
        /** TODO:
        file.open(path, ios::out | ios::binary);

        buffer.resize(8192);
        offset = 0;
        data_size = 0;

        static const uint8_t header[] = {
            0x52, 0x49, 0x46, 0x46, 0x24, 0xb0, 0x2b, 
            0x00, 0x57, 0x41, 0x56, 0x45, 0x66, 0x6d, 0x74, 0x20, 0x10, 0x00, 
            0x00, 0x00, 0x01, 0x00, 0x02, 0x00, 0x80, 0xbb, 0x00, 0x00, 0x00, 
            0xee, 0x02, 0x00, 0x04, 0x00, 0x10, 0x00, 0x64, 0x61, 0x74, 0x61, 
            0x00, 0xb0, 0x2b, 0x00, };

        file.write((char *)header, sizeof(header));
        length_pos = sizeof(header)/sizeof(header[0]) - 4;
        */
    }

    int record_sample(float left, float right)
    {
        buffer[offset++] = (int) (left * 32767 + 0.5);
        buffer[offset++] = (int) (right * 32767 + 0.5);
        if (offset >= buffer.size()) {
    ///        file.write((const char *)buffer.data(), sizeof(uint16_t) * buffer.size());
            offset = 0;
        }
        data_size += 4;
        return 0;
    }

    int record_buffer(const float * fstream, const size_t count)
    {
        for (size_t i = 0; i < count; i += 2) {
            record_sample(fstream[i], fstream[i+1]);
        }
        return count;
    }

    void close() { /***
        if (file.is_open()) {
            if (offset > 0) {
                file.write((const char *)buffer.data(), sizeof(uint16_t) * offset);
            }
            file.seekp(length_pos);
            file.write((const char *)&data_size, 4);
            printf("WavRecorder.close(): data size=%lx\n", data_size);
            file.close();
        }*/
    }

    bool recording() const {
///        return file.is_open();
    }

    ~WavRecorder()
    {
        close();
    }
};
