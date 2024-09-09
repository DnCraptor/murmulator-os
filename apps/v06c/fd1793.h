// fd1793.h for vector06sdl by Viacheslav Slavinsky, ported from vector06js
//
// Originally based upon:
// fd1793.js from Videoton TV-Computer project by Bela Babik
// https://github.com/teki/jstvc

#pragma once

#include "options.h"
#include "util.h"
#include "fsimage.h"
#include "m-os-api-cpp-unique_ptr.h"

struct DiskImage
{
    virtual uint8_t get(int i) { return 0; }
    virtual void set(int /*index*/, uint8_t /*value*/) {}
    virtual size_t size() const { return 0; }
    virtual void flush() {}
    virtual ~DiskImage() {}
};

struct CardboardDiskImage : public DiskImage
{
};

class DetachedDiskImage : public DiskImage
{
    vector<uint8_t> _data;
public:
    DetachedDiskImage() = delete;

    DetachedDiskImage(const vector<uint8_t> & data) 
        : _data(data) // allocating and copying 800KB is too slow for the main thread
    {
        uint32_t csum = 0, _csum = 0;
        for (int i = 0; i < data.size(); ++i) {
            csum += data[i];
            _csum += _data[i];
        }
        printf("DetachedDiskImage:: data=%p, _data=%p csum=%08lx _csum=%08lx\n", data.data(), _data.data(), csum, _csum);
    }

    ~DetachedDiskImage() override {
        #if SAVE_FDD
        FILE * dump = fopen(".saved.fdd", "wb");
        if (dump) {
            fwrite(&_data[0], 1, _data.size(), dump);
        }
        fclose(dump);
        #endif
        printf("~DetachedDiskImage\n");
    }

    uint8_t get(int index) override
    {
///        if (index < 0 || index >= (int)size()) 
///            throw std::out_of_range("boundary error");
        return _data[index]; 
    }

    void set(int index, uint8_t value) override
    {
///        if (index < 0 || index >= (int)size()) 
///            throw std::out_of_range("boundary error");
        _data[index] = value;
    }

    size_t size() const override {
        return _data.size();
    }
};

class FileDiskImage : public DiskImage
{
    string filename;
    vector<uint8_t> _data;
    bool loaded;
    bool dirty;
public:
    FileDiskImage(const string & path) 
        : filename(path), loaded(false), dirty(false)
    {
    }

    ~FileDiskImage()
    {
        flush();
    }

    uint8_t get(int index) override
    {
///        if (index < 0 || index >= (int)size()) 
///            throw std::out_of_range("boundary error");
        return _data[index]; 
    }

    void set(int index, uint8_t value) override
    {
///        if (index < 0 || index >= (int)size()) 
///            throw std::out_of_range("boundary error");
        bool changed = _data[index] != value;
        if (changed) {
            _data[index] = value;
            dirty = true;
        }
    }

    size_t size() const override {
        const_cast<FileDiskImage*>(this)->load();
        return _data.size();
    }
    
    void load()
    {
        if (!loaded) {
            _data = util::load_binfile(filename);
            loaded = true;
        }
    }

    void flush() override
    {
        if (dirty) {
            string tmp = util::tmpname(filename);
            FIL * copy = new FIL();
            if (f_open(copy, tmp.c_str(), FA_WRITE | FA_CREATE_ALWAYS) == FR_OK) {
                UINT sz;
                f_write(copy, &_data[0], _data.size(), &sz);
                f_close(copy);
            }
            delete copy;

            if (0 == util::careful_rename(tmp, filename)) {
                dirty = false;
            }
        }
    }

    bool is_valid()
    {
        load();
        return _data.size() > 0;
    }
};

class DirectoryImage : public DiskImage
{
    string path;
    string dirimage;
    FilesystemImage dir;
    
    bool loaded;
    bool dirty;
public:
    DirectoryImage(const string & path) 
        : path(path), dir(FilesystemImage::MAX_FS_BYTES), 
          loaded(false), dirty(false)
    {}

    ~DirectoryImage()
    {
        flush();
    }

    uint8_t get(int index) override
    {
///        if (index < 0 || index >= (int)size()) 
///            throw std::out_of_range("boundary error");
        return dir.data()[index]; 
    }

    void set(int index, uint8_t value) override
    {
///        if (index < 0 || index >= (int)size()) 
///            throw std::out_of_range("boundary error");
        bool changed = dir.data()[index] != value;
        if (changed) {
            dir.data()[index] = value;
            dirty = true;
        }
    }

    size_t size() const override {
        const_cast<DirectoryImage*>(this)->load();
        return dir.data().size();
    }
    
    void load()
    {
        if (!loaded) {
            dir.mount_local_dir(path);
            loaded = true;

            dirimage = path + "/" + "dirimage.fdd";
            dirty = true;
            flush();
        }
    }

    void flush() override
    {
        if (dirty) {
            string tmp = util::tmpname(dirimage);
            FIL * copy = new FIL();
            if (f_open(copy, tmp.c_str(), FA_WRITE | FA_CREATE_ALWAYS) == FR_OK) {
                UINT sz;
                f_write(copy, &dir.data()[0], dir.data().size(), &sz);
                f_close(copy);
            } else {
                fprintf(get_stderr(), "%s: failed to write to %s\n", __PRETTY_FUNCTION__, tmp.c_str());
            }
            delete copy;

            if (0 == util::careful_rename(tmp, dirimage)) {
                dirty = false;
            }
        }
    }
};


class FDisk
{
private:
    string name;
public:
    unique_ptr<DiskImage> dsk;
    bool error;
private:
    int sectorsPerTrack;
    int sectorSize;
    int totSec;
    int numHeads;
    int tracksPerSide;
    int data;
    int track;
    int side;
    int sector;
    int position;
    int readOffset;
    int readLength;
    int readSource;
    uint8_t readBuffer[6];


public:
    FDisk() : dsk(make_unique<CardboardDiskImage>())
    {
        this->init();
    }

    void attach(const vector<uint8_t> & data) 
    {
        this->dsk = make_unique<DetachedDiskImage>(data);
        this->init();
    }

    void attach(const string & file)
    {
        this->dsk = make_unique<FileDiskImage>(file);
        if (this->dsk->size() == 0) {
            this->dsk = make_unique<DirectoryImage>(file);
        }
        this->init();
    }

    void attach(unique_ptr<DiskImage> dsk) {
        this->dsk = dsk; // TODO:move(dsk);
        this->init();
    }

    void init()
    {
        this->sectorsPerTrack = 9;
        this->sectorSize = 512;
        this->totSec = 720;
        this->numHeads = 1;
        this->tracksPerSide = (this->totSec / this->sectorsPerTrack / this->numHeads) | 0;
        this->data = 0;
        this->track = 0;
        this->side = 0;
        this->position = 0;
        // read helpers
        this->readOffset = 0;
        this->readLength = 0;
        this->readSource = 0; // 0: dsk, 1: readBuffer
        this->error = false;
        this->parse_v06c();
    }

    static int sector_length_to_code(int length)
    {
        switch (length) {
            case 128:   return 0;
            case 256:   return 1;
            case 512:   return 2;
            case 1024:  return 3;
        }
        printf("FD1793 bad sector length %d\n", length);
        return -1;
    }

    bool isReady() const 
    {
        return this->dsk->size() > 0;
    }

    void seek(int track, int sector, int side)
    {
        if (isReady()) {
            int offsetSector = (sector != 0) ? (sector - 1) : 0;
            this->position = (track * (this->sectorsPerTrack * this->numHeads) 
                + (this->sectorsPerTrack * side) + offsetSector) * this->sectorSize;
            this->track = track;
            this->side = side;
            if (Options.log.fdc) {
                printf("FD1793: disk seek position: %08x "
                        "(side:%d,trk:%d,sec:%d)\n", this->position, this->side,
                        this->track, sector);
            }
        }
    };

    void readSector(int sector)
    {
        this->readLength = this->sectorSize;
        this->readOffset = 0;
        this->readSource = 0;
        this->sector = sector;
        this->seek(this->track, this->sector, this->side);
    }

    void readAddress()
    {
        this->readLength = 6;
        this->readSource = 1;
        this->readOffset = 0;
        this->readBuffer[0] = this->track;
        this->readBuffer[1] = this->side; // invert side ? not sure
        this->readBuffer[2] = this->sector;
        this->readBuffer[3] = FDisk::sector_length_to_code(this->sectorSize);
        this->readBuffer[4] = 0;
        this->readBuffer[5] = 0;
    }

    bool read()
    {
        this->error = false;
        bool finished = true;
        if (this->readOffset < this->readLength) {
            finished = false;
            if (this->readSource) {
                this->data = this->readBuffer[this->readOffset];
            } else {
        ///   TODO:     try {
                    this->data = this->dsk->get(this->position + this->readOffset);
        ///        } catch(...) {
        ///            printf("FD1793: exception in dsk->get(%d) out of bounds?\n", this->position + this->readOffset);
        ///            this->error = true;
        ///        }
            }
            this->readOffset++;
        } else {
            if (Options.log.fdc) {
                printf("FD1793: read finished src:%d\n", this->readSource);
            }
        }
        //printf("FD1793: disk read, rem: %d finished: %d\n", this->readLength,
        //        finished);
        return finished;
    }

    bool write_data(uint8_t data)
    {
        bool finished = true;
        if (this->readOffset < this->readLength) {
            finished = false;
            this->dsk->set(this->position + this->readOffset, data);
            this->readOffset++;
            if (this->readOffset == this->readLength) {
                this->dsk->flush();
            }
        }
        else {
            if (Options.log.fdc) {
                printf("FD1793: write finished: position=%08x\n", this->position);

                for (int i = 0; i < this->readLength; ++i) {
                    printf("%02x ", this->dsk->get(this->position + i));

                    if ((i + 1) % 16 == 0 || i + 1 == this->readLength) {
                        printf("  ");
                        for (int j = -15; j <= 0; ++j) {
                            printf("%c", util::printable_char(
                                        this->dsk->get(this->position + i + j)));
                        }
                        printf("  W\n");
                    }
                }
            }
        }
        return finished;
    }

    void writeSector(int sector)
    {
        this->readLength = this->sectorSize;
        this->readOffset = 0;
        this->readSource = 0;
        this->sector = sector;
        this->seek(this->track, this->sector, this->side);
    }

    // Vector-06c floppy: 2 sides, 5 sectors of 1024 bytes
    bool parse_v06c() 
    {
        const int FDD_NSECTORS = 5;

        if (!this->isReady()) {
            return false;
        }
        this->tracksPerSide = (this->dsk->size() >> 10) / 2 * FDD_NSECTORS;
        this->numHeads = 2;
        this->sectorSize = 1024;
        this->sectorsPerTrack = 5;
        this->totSec = this->dsk->size() / 1024;

        return true;
    };

    uint8_t read_data() const
    {
        return this->data;
    }

};


class FD1793
{
private:
    FDisk _disks[4];
    int _pr;    // port 4, parameter register: SS,MON,DDEN,HLD,DS3,DS2,DS1,DS0
                // motor on: 1: motor on
                // double density: 1: on
                // hold: 1: head on disk (it is or-ed with motor on)
                // drive select: 1: drive active
    int _side;  // side select: 0: side 0, 1: side 1
    int _intrq;
    int _data;  // data
    int _status;
    int _command;
    int _commandtr;
    int _track;
    int _sector;
    int _stepdir;
    int _lingertime;
    int _dsksel;

    int sectorcnt = 0;
    int readcnt = 0;
    int fault = 0;

    int LINGER_BEFORE = 0;
    int LINGER_AFTER = 0;

    char debug_buf[16];
    int debug_n;

    FDisk & dsk(int n = -1) {
        return _disks[n == -1 ? _dsksel : n];
    }

public:
    const int ST_NOTREADY = 0x80; // sampled before read/write
    const int ST_READONLY = 0x40;
    const int ST_HEADLOADED = 0x20;
    const int ST_RECTYPE = 0x20;
    const int ST_WRFAULT = 0x20;
    const int ST_SEEKERR = 0x10;
    const int ST_RECNF = 0x10;
    const int ST_CRCERR = 0x08;
    const int ST_TRACK0 = 0x04;
    const int ST_LOSTDATA = 0x04;
    const int ST_INDEX = 0x02;
    const int ST_DRQ = 0x02;
    const int ST_BUSY = 0x01;

    const int PRT_INTRQ = 0x01;
    const int PRT_DRQ = 0x80;

    const int CMD_READSEC = 1;
    const int CMD_READADDR = 2;
    const int CMD_WRITESEC = 3;


    // Delays, not yet implemented:
    //   A command and between the next status: mfm 14us, fm 28us
    // Reset
    //  - registers cleared
    //  - restore (03) command
    //  - steps until !TRO0 goes low (track 0)
    //
    FD1793() : _disks{}, _dsksel(0)
    {
        LINGER_BEFORE = 0;
        LINGER_AFTER = 2;
        _lingertime = 3;
        _stepdir = 1;
        _status = 0;
    }

    void init()
    {
        _status = 0;
    }

    FDisk & disk(int drive) {
///        if (drive < 0 || drive > 3) {
///            throw std::out_of_range("drives must be in range 0..3");
//        }
        return this->_disks[drive];
    }

    void exec() 
    {
        bool finished;
        if (this->_commandtr == CMD_READSEC || this->_commandtr == CMD_READADDR) 
        {
            if (this->_status & ST_DRQ) {
                printf("FD1793: invalid read\n");
                return;
            }
            finished = this->dsk().read();
            if (this->dsk().error) {
                this->_status |= ST_CRCERR;
                this->_status &= ~ST_BUSY;
                this->_status &= ~ST_DRQ;
            }
            else if (finished) {
                this->_status &= ~ST_BUSY;
                this->_intrq = PRT_INTRQ;
            } else {
                this->_status |= ST_DRQ;
                this->_data = this->dsk().read_data();
            }
            //if (Options.log.fdc) {
            //    printf("FD1793: exec - read done, "
            //            "finished: %d data: %02x status: %02x\n", 
            //            finished, this->_data, this->_status);
            //}
        }
        else if (this->_commandtr == CMD_WRITESEC) {
            if ((this->_status & ST_DRQ) == 0) {
                finished = this->dsk().write_data(this->_data);
                if (finished) {
                    this->_status &= ~ST_BUSY;
                    this->_intrq = PRT_INTRQ;
                }
                else {
                    this->_status |= ST_DRQ;
                }
            }
        } 
        else {
            // finish lingering
            this->_status &= ~ST_BUSY;
        }
    }

    int prevstatus = -1;

    int read(int addr)
    {
        if (Options.nofdc) return 0xff;
        int result = 0;
        if (this->dsk().isReady()) 
            this->_status &= ~ST_NOTREADY;
        else 
            this->_status |= ST_NOTREADY;
        int returnStatus;
        switch (addr) {
            case 0: // status
                // if ((this->_status & ST_BUSY) && this->_lingertime > 0) {
                //         if (--this->_lingertime == 0) {
                //             this->exec();
                //         }
                // }
                // to make software that waits for the controller to start happy:
                // linger -10 to 0 before setting busy flag, set busy flag
                // linger 0 to 10 before exec
                returnStatus = this->_status;
                if (this->_status & ST_BUSY) {
                    if (this->_lingertime < 0) {
                        returnStatus &= ~ST_BUSY; // pretend that we're slow 
                        ++this->_lingertime;
                    } else if (this->_lingertime < this->LINGER_AFTER) {
                        ++this->_lingertime;
                    } else if (this->_lingertime == this->LINGER_AFTER) {
                        ++this->_lingertime;
                        this->exec();
                        returnStatus = this->_status;
                    }
                }

                //this->_status &= (ST_BUSY | ST_NOTREADY);
                this->_intrq = 0;
                
                //if (this->fault) {
                //    printf("read status: injecting fault: result= ff\n");
                //    returnStatus = 0xff;
                //}

                result = returnStatus;
                if (Options.log.fdc) {
                    if (prevstatus != result) {
                        printf("[%02x]", result);
                        prevstatus = result;
                    }
                }
                this->_status &= ~ST_CRCERR;
                break;

            case 1: // track
                result = this->_track;
                break;

            case 2: // sector
                result = this->_sector;
                break;

            case 3: // data
                if (!(this->_status & ST_DRQ)) { //throw ("invalid read");
                    if (Options.log.fdc) {
                        printf("FD1793: reading too much!\n");
                    }
                }
                result = this->_data;
                ++this->readcnt;
                if (this->readcnt == 1024) {
                    ++this->sectorcnt;
                }
                #if 1
                if (Options.log.fdc) {
                    if (this->_status & ST_DRQ) {
                        printf("%02x ", result);
                        this->debug_buf[this->debug_n] = result;
                        if (++this->debug_n == 16) {
                            printf("  ");
                            this->debug_n = 0;
                            for (int i = 0; i < 16; ++i) {
                                printf("%c", util::printable_char(
                                            this->debug_buf[i]));
                            }
                            printf("\n");
                        }
                    }
                }
                #endif
                this->_status &= ~ST_DRQ;
                this->exec();
                //console.log("FD1793: read data:",Utils.toHex8(result));
                break;

            case 4:
                if (this->_status & ST_BUSY) {
                    this->exec();
                }
                // DRQ,0,0,0,0,0,0,INTRQ
                // faster to use than FDC
                result = this->_intrq | ((this->_status & ST_DRQ) ? PRT_DRQ : 0);
                break;

            default:
                printf("FD1793: invalid port read\n");
                result = -1;
                break;

        }
        //if (!(this->_status & ST_DRQ) && Options.log.fdc) {
        //    printf("FD1793: read port: %02x result: %02x status: %02x\n", addr, 
        //            result, this->_status);
        //}
        return result;
    };

    void command(int val)
    {
        int cmd = val >> 4;
        int param = val & 0x0f;
        int update, multiple;
        update = multiple = (param & 1);
        this->_intrq = 0;
        this->_command = val;
        this->_commandtr = 0;
        switch (cmd) {
            case 0x00: // restor, type 1
                if (Options.log.fdc) {
                    printf("CMD restore\n");
                }
                this->_intrq = PRT_INTRQ;
                if (this->dsk().isReady()) {
                    this->_track = 0;
                    this->dsk().seek(this->_track, 1, this->_side);
                } else {
                    this->_status |= ST_SEEKERR;
                }
                break;
            case 0x01: // seek
                if (Options.log.fdc) {
                    printf("CMD seek: %02x\n", param);
                }
                this->dsk().seek(this->_data, this->_sector, this->_side);
                this->_track = this->_data;
                this->_intrq = PRT_INTRQ;
                this->_status |= ST_BUSY;
                this->_lingertime = this->LINGER_BEFORE;
                break;
            case 0x02: // step, u = 0
            case 0x03: // step, u = 1
                if (Options.log.fdc) {
                    printf("CMD step: update=%d\n", update);
                }
                this->_track += this->_stepdir;
                if (this->_track < 0) {
                    this->_track = 0;
                }
                this->_lingertime = this->LINGER_BEFORE;
                this->_status |= ST_BUSY;
                break;
            case 0x04: // step in, u = 0
            case 0x05: // step in, u = 1
                if (Options.log.fdc) {
                    printf("CMD step in: update=%d\n", update);
                }
                this->_stepdir = 1;
                this->_track += this->_stepdir;
                this->_lingertime = this->LINGER_BEFORE;
                this->_status |= ST_BUSY;
                //this->dsk().seek(this->_track, this->_sector, this->_side);
                break;
            case 0x06: // step out, u = 0
            case 0x07: // step out, u = 1
                if (Options.log.fdc) {
                    printf("CMD step out: update=%d\n", update);
                }
                this->_stepdir = -1;
                this->_track += this->_stepdir;
                if (this->_track < 0) {
                    this->_track = 0;
                }
                this->_lingertime = this->LINGER_BEFORE;
                this->_status |= ST_BUSY;
                break;
            case 0x08: // read sector, m = 0
            case 0x09: // read sector, m = 1
                {
                //int rsSideCompareFlag = (param & 2) >> 1;
                //int rsDelay = (param & 4) >> 2;
                //int rsSideSelect = (param & 8) >> 3;
                this->_commandtr = CMD_READSEC;
                this->_status |= ST_BUSY;
                this->dsk().seek(this->_track, this->_sector, this->_side);
                this->dsk().readSector(this->_sector);
                this->debug_n = 0;
                this->_lingertime = this->LINGER_BEFORE;
                if (Options.log.fdc) {
                    printf("CMD read sector m:%d p:%02x sector:%d "
                            "status:%02x linger: %d\n", multiple, param, this->_sector,
                            this->_status, this->_lingertime);
                }
                }
                break;
            case 0x0A: // write sector, m = 0
            case 0x0B: // write sector, m = 1
                {
                this->_commandtr = CMD_WRITESEC;
                this->_status |= ST_BUSY;
                this->_status |= ST_DRQ;
                this->dsk().seek(this->_track, this->_sector, this->_side);
                this->dsk().writeSector(this->_sector);
                this->debug_n = 0;
                if (Options.log.fdc) {
                    printf("CMD write sector m:%d p:%02x sector:%d "
                            "status:%02x\n", multiple, param, this->_sector,
                            this->_status);
                }
                this->_lingertime = this->LINGER_BEFORE;
                }
                break;
            case 0x0C: // read address
                this->_commandtr = CMD_READADDR;
                this->_status |= ST_BUSY;
                this->dsk().readAddress();
                this->_lingertime = this->LINGER_BEFORE;
                if (Options.log.fdc) {
                    printf("CMD read address m:%d p:%02x status:%02x",
                            multiple, param, this->_status);
                }
                break;
            case 0x0D: // force interrupt
                if (Options.log.fdc) {
                    printf("CMD force interrupt\n");
                }
                break;
            case 0x0E: // read track
                printf("CMD read track (not implemented)\n");
                break;
            case 0x0F: // write track
                printf("CMD write track (not implemented)\n");
                break;
        }
        // if ((this->_status & ST_BUSY) != 0) {
        //     this->_lingertime = 10;
        // }
    };

    void write(int addr, int val)
    {
        //if (Options.log.fdc) {
        //    printf("FD1793: write [%02x]=%02x: ", addr, val);
        //}
        switch (addr) {
            case 0: // command
                if (Options.log.fdc) {
                    printf("COMMAND %02x: ", val);
                }
                this->command(val);
                break;

            case 1: // track (current track)j
                if (Options.log.fdc) {
                    printf("set track:%d\n", val);
                }
                this->_track = val;
                this->_status &= ~ST_DRQ;
                break;

            case 2: // sector (desired sector)
                if (Options.log.fdc) {
                    printf("set sector:%d\n", val);
                }
                this->_sector = val;
                this->_status &= ~ST_DRQ;
                break;

            case 3: // data
                if (Options.log.fdc) {
                    printf("set data:%02x\n", val);
                }
                this->_data = val;
                this->_status &= ~ST_DRQ;
                this->exec();
                break;

            case 4: // param reg
                this->_pr = val;
                // Kishinev v06c 
                // 0 0 1 1 x S A B
                this->_dsksel = val & 3;
                this->_side = ((~val) >> 2) & 1; // invert side
                //if (Options.log.fdc) {
                //    printf("set pr:%02x disk select: %d side: %d\n",
                //            val, val & 3, this->_side);
                //}

                // // SS,MON,DDEN,HLD,DS3,DS2,DS1,DS0
                // if (val & 1) this->dsk() = this->_disks[0];
                // else if (val & 2) this->dsk() = this->_disks[1];
                // else if (val & 4) this->dsk() = this->_disks[2];
                // else if (val & 8) this->dsk() = this->_disks[3];
                // else this->dsk() = this->_disks[0];
                // this->_side = (this->_pr & 0x80) >>> 7;
                break;
            default:
                printf("invalid port write\n");
                return;
        }
    }
};

