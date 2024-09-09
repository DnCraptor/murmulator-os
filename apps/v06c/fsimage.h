#pragma once

#include "util.h"
#include "m-os-api-cpp-unordered_set.h"
#include "m-os-api-cpp-tuples.h"

__attribute__((packed))
struct MDHeaderData
{
    uint8_t User;
    char Name[8];
    char Ext[3];
    uint8_t Extent;
    uint8_t Unknown1;
    uint8_t Unknown2;
    uint8_t Records;
    uint16_t FAT[8];
};

struct MDHeader
{
    typedef MDHeaderData datatype;

    MDHeaderData * fields;
    uint8_t * bytes;

    static constexpr size_t SIZE = sizeof(MDHeaderData);

    uint8_t user() const { return fields->User; }

    string name() const { 
        return util::rtrim_copy(string(fields->Name, 8));
    }

    string ext() const {
        return util::rtrim_copy(string(fields->Ext, 3));
    }

    MDHeader(uint8_t * bytes)
        : fields(reinterpret_cast<MDHeaderData *>(bytes)), bytes(bytes) 
    {}

    void init_with_filename(string stem, string ext);
    void overwrite(const MDHeader & other);
    bool operator==(const MDHeader & rhs) const;
    bool operator!=(const MDHeader & rhs) const;
};

struct Dirent
{
    MDHeader header;
    vector<uint16_t> chain;
    int size;

    Dirent() :header(nullptr), size(0) {}

    Dirent(MDHeader header) : header(header), size(0) { }

    Dirent(const Dirent& rhs)
        : header(rhs.header), chain(rhs.chain), size(rhs.size) {
    }

    string name() const { return header.name(); }

    string ext() const { return header.ext(); }

    uint8_t user() const { return header.user(); }
};

class FilesystemImage {
    friend struct FilesystemTest;

    typedef vector<uint8_t> bytes_t;
    typedef vector<uint16_t> chain_t;
    unordered_set<string> taken_names;
    bytes_t bytes;

    chain_t build_available_chain(int length);
    string allocate_file(const string & filename,
            const bytes_t & content, const chain_t & chain);
    triple<int,int,int> cluster_to_ths(int cluster) const;
    bytes_t::iterator map_sector(int track, int head, int sector);
    // load full file information and sector map
    Dirent load_dirent(MDHeader header);
    bytes_t read_bytes(const Dirent & de);

    duple<string, string>
        unique_cpm_filename(const string & filename);

public:
    static constexpr int SECTOR_SIZE = 1024;
    static constexpr int SECTORS_CYL = 10;
    static constexpr int MAX_FS_BYTES = 860160;
    static constexpr int CLUSTER_BYTES = 2048;
    static constexpr int SYSTEM_TRACKS = 6;

    struct dir_iterator {
        FilesystemImage & fs;
        int position;

        dir_iterator(FilesystemImage & fs, int position) 
            : fs(fs), position(position) {
        }

        MDHeader operator*() const {
            return MDHeader(&fs.bytes[position]);
        }

        const dir_iterator& operator++() { // prefix
            position += 32;
            return *this;
        }

        bool operator==(const dir_iterator& rhs) {
            return position == rhs.position;
        }

        bool operator!=(const dir_iterator& rhs) {
            return position != rhs.position;
        }
    };

    FilesystemImage() : FilesystemImage(0) {}

    FilesystemImage(int size) : bytes(size, 0xe5) {}

    FilesystemImage(const bytes_t & data) : bytes(data) 
    {
    }

    bool mount_local_dir(string path);

    void set_data(const bytes_t & data);
    bytes_t& data() { return bytes; }
    const bytes_t& data() const { return bytes; }

    dir_iterator begin();
    dir_iterator end();

    Dirent find_file(const string & filename);
    void listdir(/**TODO: std::function<void(const Dirent &)> cb*/);

    bytes_t read_file(const string & filename);

    // return ok, internal file name
    duple<bool,string> 
        save_file(const string & filename, bytes_t content);
};

