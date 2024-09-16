//////////////////////////////////////////////////
//                                              //
// Emu64                                        //
// von Thorsten Kattanek                        //
//                                              //
// #file: c64_class.h                           //
//                                              //
// Dieser Sourcecode ist Copyright geschützt!   //
// Geistiges Eigentum von Th.Kattanek           //
//                                              //
// www.emu64.de                                 //
//                                              //
//////////////////////////////////////////////////

#ifndef C64CLASS_H
#define C64CLASS_H

typedef float  float_t;
#define FILENAME_MAX 64

#include "m-os-api-cpp-string.h"

#include "structs.h"
///#include "./video_crt_class.h"
#include "./mmu_class.h"
#include "./mos6510_class.h"
#include "./mos6569_class.h"
#include "./mos6581_8085_class.h"
#include "./mos6526_class.h"
#include "./cartridge_class.h"
#include "./reu_class.h"
#include "./georam_class.h"
#include "./floppy1541_class.h"
#include "./tape1530_class.h"
///#include "./cpu_info.h"
///#include "./vcd_class.h"
///#include "./savepng.h"
///#include "./video_capture_class.h"

/**
#include <GL/glu.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_image.h>
#include <functional>
*/

#define AudioSampleRate 44100

#define MAX_STRING_LENGTH 1024

#define MAX_FLOPPY_NUM 4
#define MAX_BREAK_GROUP_NUM 255
#define MAX_SDL_JOYSTICK_NUM 16
#define MAX_VJOY_NUM 16

#define SUBDIVS_SCREEN 20             // Für Screenverzerrungen (unterteilung des Bildschirms)

#define SCREEN_RATIO_4_3 1.34f        // Screenratio 4:3 (1,33333)
#define SCREEN_RATIO_5_4 1.25f        // Screenratio 5:4 (1,25)
#define SCREEN_RATIO_16_9 1.777f      // Screenratio 16:9 (1,777)

#define MAX_VIDEO_DISPLAYS 8				  // Anzahl der Maximal unterstützen Video Displays

enum SCREENSHOT_FORMATS {SCREENSHOT_FORMAT_BMP, SCREENSHOT_FORMAT_PNG, SCREENSHOT_FORMATS_COUNT};

class C64Class
{
    string debug;
public:
    C64Class(
        int *ret_error,
        int soundbuffer_size,
///        VideoCrtClass *video_crt_output,
///        bool start_minimized,
///        std::function<void(const char*)> log_function,
        const char *data_path
    );
    ~C64Class();
    void StartEmulation();
    void EndEmulation();
///    void SetLimitCycles(int nCycles);
    void SetEnableDebugCart(bool enable);
    void WarpModeLoop();
    int GetAudioSampleRate();
    void FillAudioBuffer(uint8_t *stream, int laenge); // Über diese Funktion wird der C64 Takt erzeugt !! //
    void KeyEvent(uint8_t  matrix_code, KeyStatus status, bool isAutoShift);
    bool LoadC64Roms(const char *kernalrom, const char *basicrom, const char *charrom);
    bool LoadFloppyRom(uint8_t floppy_nr, const char *dos1541rom);
	bool LoadDiskImage(uint8_t floppy_nr, FILE *file, int typ);
    void LoadPRGFromD64(uint8_t floppy_nr, char *c64_filename, int command);
    void SetFloppyWriteProtect(uint8_t floppy_nr, bool status);
    void SetCommandLine(const string& c64_command);
    void KillCommandLine();
    uint8_t ReadC64Byte(uint16_t address);
    void WriteC64Byte(uint16_t address, uint8_t value);
    uint8_t* GetRAMPointer(uint16_t address);
///    void SetGrafikModi(bool enable_screen_doublesize, bool enable_screen_crt_output, bool filter_enable, uint16_t fullscreen_width = 0, uint16_t fullscreen_height = 0);
    void SetSDLWindowName(const char *name);
///    void SetFullscreen(bool is_fullscreen);
///    void ToggleScreenMode();
    void InitGrafik();
///    void ChangeVSync();
    void ReleaseGrafik();
    void DrawC64Screen();
///    void SetFocusToC64Window();
///    void SetWindowAspectRatio(bool enable);
///    void SetVSync(bool enable);
///    void SetFullscreenAspectRatio(bool enable);
///    void AnalyzeSDLEvent(SDL_Event *event);
    void SetC64Frequency(int c64_frequency);
    void SetC64Speed(int speed);
    void EnableWarpMode(bool enabled);
	void ToggleWarpMode();
	bool IsWarpMode();
///    void SetDistortion(float_t value);
    void SetMouseHiddenTime(int time);  // Time in ms // Bei 0 Wird der Cursor nicht mehr ausgeblendet
    void GetWindowPos(int *x, int *y);
    void SetWindowPos(int x, int y);
    void GetWindowSize(int *x, int *y);
///    void SetWindowSize(int w, int h);
	int GetNumDisplays();
	const char* GetDisplayName(int display_index);
	int GetNumDisplayModes(int display_index);
///	int GetDisplayMode(int display_index, int mode_index, int &w, int &h, int &refresh_rate, uint32_t &format);
///	void SetFullscreenDisplayMode(int display_index, int mode_index);

	bool LoadTapeImage(FILE *file, int typ);
	bool RecordTapeImage(FILE* file);
    uint8_t SetTapeKeys(uint8_t pressed_key);
    bool GetTapeMotorStatus();
    bool GetTapeRecordLedStatus();
    uint32_t GetTapeCounter();
    float_t GetTapeLenTime();
    uint32_t GetTapeLenCount();
    void SetTapeSoundVolume(float_t volume);

    void SoftReset();
    void HardReset();
    void SetReset(int status, int hard_reset);
	int LoadAutoRun(uint8_t floppy_nr, FILE *file, const char *filename, int typ);
	int LoadPRG(FILE *file, const char* filename, int typ, uint16_t *return_start_address);	// Filename ist nur für Logausgabe / Ausschlaggebend ist FILE*

	int LoadCRT(FILE *file);
    void RemoveCRT();
	int CreateNewEasyFlashImage(FILE *file, const char *crt_name);

    void InsertREU();
    void RemoveREU();
    int LoadREUImage(const char *filename);
    int SaveREUImage(const char *filename);
    void ClearREURam();

    void InsertGeoRam();
    void RemoveGeoRam();
    int LoadGeoRamImage(const char *filename);
    int SaveGeoRamImage(const char *filename);
    void ClearGeoRam();
    uint8_t GetGeoRamMode();
    void SetGeoRamMode(uint8_t mode);

    void SetMouse1351Port(uint8_t port);

    void ResetC64CycleCounter();
    void SetDebugMode(bool status);
    void SetCpuExtLines(bool status);
    void SetExtRDY(bool status);
    void OneCycle();
    void OneOpcode(int source);
///    void SetDebugAnimation(bool status);
///    void SetDebugAnimationSpeed(int cycle_sek);
    void GetC64CpuReg(REG_STRUCT *reg,IREG_STRUCT *ireg);
    void GetVicReg(VIC_STRUCT *vic_reg);
    void GetIECStatus(IEC_STRUCT *iec);
    bool StartDebugLogging(const char *filename);
    void StopDebugLogging();
    int Disassemble(FILE* file, uint16_t pc, bool line_draw);
/**
    int AddBreakGroup();
    void DelBreakGroup(int index);
    BREAK_GROUP* GetBreakGroup(int index);
    void UpdateBreakGroup();
    void DeleteAllBreakGroups();
    int GetBreakGroupCount();
    int LoadBreakGroups(const char *filename);
    bool SaveBreakGroups(const char *filename);
*/
    bool ExportPRG(const char *filename, uint16_t start_adresse, uint16_t end_adresse, int source);
    bool ExportRAW(const char *filename, uint16_t start_adresse, uint16_t end_adresse, int source);
    bool ExportASM(const char *filename, uint16_t start_adresse, uint16_t end_adresse, int source);
    void JoystickNewScan();
    void StartRecJoystickMapping(int slot_nr);
    void StopRecJoystickMapping();
    void ClearJoystickMapping(int slot_nr);
	void SwapJoyPorts();
    void IncMouseHiddenCounter();

    void StartRecKeyMap(uint8_t keymatrix_code);
    void StopRecKeyMap();
    bool GetRecKeyMapStatus();
    /**
    C64_KEYS* GetC64KeyTable();
    const char** GetC64KeyNameTable();
    int GetC64KeyTableSize();
    */

    uint8_t GetMapReadSource(uint8_t page);
    uint8_t GetMapWriteDestination(uint8_t page);
/**
    void SetScreenshotNumber(uint32_t number);
    uint32_t GetScreenshotNumber();
    void SetScreenshotFormat(uint8_t format);
    void SetScreenshotDir(const char* screenshot_dir);
    void EnableScreenshots(bool is_enable);
    void SaveScreenshot();
    const char* GetScreenshotFormatName(uint8_t format);
    void SetExitScreenshot(const char *filename);

    const char* GetAVVersion();
    bool StartVideoRecord(const char *filename, int audio_bitrate=128000, int video_bitrate=4000000);
    void SetPauseVideoRecord(bool status);
    void StopVideoRecord();
    int GetRecordedFrameCount();

    bool StartIECDump(const char *filename);
    void StopIECDump();
*/

    void SetSIDVolume(float_t volume);  // Lautstärke der SID's (0.0f - 1.0f)
    void SetFirstSidTyp(int sid_typ);   // SID Typ des 1. SID (MOS_6581 oder MOS_8580)
    void SetSecondSidTyp(int sid_typ);  // SID Typ des 2. SID (MOS_6581 oder MOS_8580)
    void EnableStereoSid(bool enable);  // 2. SID aktivieren
    void SetStereoSidAddress(uint16_t address);
    void SetStereoSid6ChannelMode(bool enable);
    void SetSidCycleExact(bool enable);
    void SetSidFilter(bool enable);
/**
    bool StartSidDump(const char *filename);
    void StopSidDump();
    int GetSidDumpFrames();
*/
    void SetVicConfig(int var, bool enable);    // var = VIC_BORDER_ON, VIC_SPRITES_ON, VIC_SPR_SPR_COLL_ON, VIC_SPR_BCK_COLL_ON
    bool GetVicConfig(int var);

    void SetVicDisplaySizePal(uint16_t first_line, uint16_t last_line);
    void SetVicDisplaySizeNtsc(uint16_t first_line, uint16_t last_line);

    int GetVicFirstDisplayLinePal();
    int GetVicLastDisplayLinePal();
    int GetVicFirstDisplayLineNtsc();
    int GetVicLastDisplayLineNtsc();

///    static const char* screenshot_format_name[SCREENSHOT_FORMATS_COUNT];

///    bool            start_minimized;
///	bool			start_hidden_window;

    uint16_t        current_window_width;
    uint16_t        current_window_height;
    int             current_window_color_bits;
    uint16_t        current_c64_screen_width;
    uint16_t        current_c64_screen_height;
///    bool            enable_screen_doublesize;
///    bool            enable_screen_crt_output;
///    bool            enable_screen_filter;
///    uint16_t        fullscreen_width;
///    uint16_t        fullscreen_height;
///    bool            enable_fullscreen;
///    bool            changed_graphic_modi;
///    bool            enable_vsync;
///    bool            changed_vsync;
///    bool            changed_window_pos;
///    bool            changed_window_size;
    bool            enable_hold_vic_refresh;
    bool            vic_refresh_is_holded;

///    float_t         screen_aspect_ratio;
///    bool            enable_window_aspect_ratio;
///    bool            enable_fullscreen_aspect_ratio;

///    SDL_Window      *sdl_window;
///    SDL_Surface     *sdl_window_icon;
///    SDL_GLContext   gl_context;
///    SDL_DisplayMode fullscreen_display_mode[MAX_VIDEO_DISPLAYS];

///    int             sdl_window_pos_x;
///    int             sdl_window_pos_y;
///    int             sdl_window_size_width;
///    int             sdl_window_size_height;

///    SDL_mutex       *mutex1;  // Dient für das füllen des Soundbuffers
///    SDL_AudioDeviceID audio_dev;
///    SDL_AudioSpec   audio_spec_want;
///    SDL_AudioSpec   audio_spec_have;

    int             audio_frequency;
    uint16_t        audio_sample_bit_size;
    uint16_t        audio_channels;
    bool            is_audio_sample_little_endian;
    bool            is_audio_sample_float;
    bool            is_audio_sample_signed;

    int16_t         *audio_16bit_buffer;
    int             audio_16bit_buffer_size;

///    SDL_Surface     *c64_screen;
///    GLuint          c64_screen_texture;
///    uint8_t         *c64_screen_buffer;
    bool            c64_screen_is_obselete;

///    bool            enable_distortion;
///    float_t         distortion_value;

    /// Distortion (Verzerrung) ///
///    POINT_STRUCT    distortion_grid_points[(SUBDIVS_SCREEN+1)*(SUBDIVS_SCREEN+1)];
///    POINT_STRUCT    distortion_grid[(SUBDIVS_SCREEN)*(SUBDIVS_SCREEN)*4];
///    POINT_STRUCT    distortion_grid_texture_coordinates[(SUBDIVS_SCREEN)*(SUBDIVS_SCREEN)*4];

///    int				frame_skip_counter;

///    SDL_Surface     *img_joy_arrow0;
///    SDL_Surface     *img_joy_arrow1;
///    SDL_Surface     *img_joy_button0;
///    SDL_Surface     *img_joy_button1;

///    GLuint          texture_joy_arrow0;
///    GLuint          texture_joy_arrow1;
///    GLuint          texture_joy_button0;
///    GLuint          texture_joy_button1;

    bool            rec_joy_mapping;
    int             rec_joy_mapping_pos;          // 0-4 // Hoch - Runter - Links - Rechts - Feuer
    int             rec_joy_slot_nr;              // 0 - (MAX_VJOYS-1)
    int             rec_polling_wait;
    int             rec_polling_wait_counter;

///    VIRTUAL_JOY_STRUCT  virtual_joys[MAX_VJOY_NUM];
    int                 virtual_port1;
    int                 virtual_port2;

    uint8_t         game_port1;
    uint8_t         game_port2;

    TaskHandle_t    sdl_thread;
    bool            sdl_thread_pause;
    bool            sdl_thread_is_paused;

    TaskHandle_t    warp_thread;
    bool            warp_thread_end;

    uint8_t         *vic_buffer;
///    VideoCrtClass   *video_crt_output;

    int             c64_frequency;   // Normaler PAL Takt ist 985248 Hz (985248 / (312 Rz * 63 Cycles) = 50,124542 Hz)
                                     // 50 Hz Syncroner Takt ist 982800 Hz

	int				c64_frequency_temp;	// Dient zum zwischenspeichern der C64 Frequenz beim Videorecord.

    int             c64_speed;

    MMU             *mmu;
    MOS6510         *cpu;
    VICII           *vic;
    MOS6581_8085    *sid1;
    MOS6581_8085    *sid2;
    MOS6526         *cia1;
    MOS6526         *cia2;
    CartridgeClass  *crt;
    REUClass        *reu;
    GEORAMClass     *geo;
    Floppy1541      *floppy[MAX_FLOPPY_NUM];
    TAPE1530        *tape;

    bool            enable_stereo_sid;
    bool            enable_stereo_sid_6channel_mode;
    uint16_t        stereo_sid_address;

    uint8_t         key_matrix_to_port_a_ext[8];
    uint8_t         key_matrix_to_port_b_ext[8];

    uint8_t         io_source;

    bool            wait_reset_ready;
    uint8_t         auto_load_mode;
    string          auto_load_command_line; //[MAX_STRING_LENGTH];
    string          auto_load_filename; //[MAX_STRING_LENGTH];
	FILE*			auto_load_file;
	int				auto_load_file_typ;

    bool            loop_thread_end;
    bool            loop_thread_is_end;
/**
    std::function<void(void)> AnimationRefreshProc;
    std::function<void(void)> BreakpointProc;
    std::function<void(const char*)> LogText;
    std::function<void(void)> CloseEventC64Screen;
    std::function<void(void)> LimitCyclesEvent;
    std::function<void(uint8_t)> DebugCartEvent;
    std::function<void(uint8_t *audio_buffer0, uint8_t *audio_buffer1, uint8_t *audio_buffer2, int length)> AudioOutProc;
*/
///    uint16_t        cpu_pc_history[256];
///    uint8_t         cpu_pc_history_pos;

///    bool            start_screenshot;
///    char            screenshot_filename[MAX_STRING_LENGTH];

///    bool            is_screenshot_enable;
///    uint32_t        screenshot_number;
///    char*           screenshot_dir;
///    uint8_t         screenshot_format;

///    bool            enable_exit_screenshot;
///    char            exit_screenshot_filename[MAX_STRING_LENGTH];

    float_t         sid_volume;

///    VideoCaptureClass *video_capture;

private:
    inline void NextSystemCycle();
///    void CalcDistortionGrid();
    void VicRefresh(uint8_t *vic_puffer);
    void CheckKeys();
    uint16_t DisAss(FILE *file, uint16_t pc, bool line_draw, int source);
///    bool CheckBreakpoints();
    void WriteSidIO(uint16_t address, uint8_t value);
    uint8_t ReadSidIO(uint16_t address);
    void WriteIO1(uint16_t address, uint8_t value);
    void WriteIO2(uint16_t address, uint8_t value);
    uint8_t ReadIO1(uint16_t address);
    uint8_t ReadIO2(uint16_t address);
///    void SDLThreadPauseBegin();
///    void SDLThreadPauseEnd();
    void OpenSDLJoystick();
    void CloseSDLJoystick();
    void ChangePOTSwitch();
    void UpdateMouse();
///    int InitVideoCaptureSystem();
///    void CloseVideoCaptureSystem();
///    void SwapRBSurface(SDL_Surface *surface); // swaps the color red with blue in sdl surface
    void DebugLogging();
    ReadProcFn<C64Class>* ReadProcTbl;
    WriteProcFn<C64Class>* WriteProcTbl;
///    char  gfx_path[MAX_STRING_LENGTH];
    string  floppy_sound_path;///[MAX_STRING_LENGTH];
    string  rom_path;///[MAX_STRING_LENGTH];

///    char sdl_window_name[MAX_STRING_LENGTH];

    bool sdl_joystick_is_open;
    int  sdl_joystick_count;
///    SDL_Joystick *sdl_joystick[MAX_SDL_JOYSTICK_NUM];
///    const char *sdl_joystick_name[MAX_SDL_JOYSTICK_NUM];
    bool sdl_joystick_stop_update;
    bool sdl_joystick_update_is_stoped;

    bool reset_wire;        // Reset Leitung -> Für Alle Module mit Reset Eingang
    bool rdy_ba_wire;       // Leitung CPU <-- VIC
    bool game_wire;         // Leitung Expansionsport --> MMU;
    bool exrom_wire;        // Leitung Expansionsport --> MMU;
    bool hi_ram_wire;       // Leitung Expansionsport --> MMU;
    bool lo_ram_wire;       // Leitung Expansionsport --> MMU;

    MOS6510_PORT cpu_port;  // Prozessor Port

    bool enable_ext_wires;
    bool ext_rdy_wire;

    uint8_t c64_iec_wire;       // Leitungen vom C64 zur Floppy Bit 4=ATN 6=CLK 7=DATA
    uint8_t floppy_iec_wire;    // Leitungen von Floppy zur c64 Bit 6=CLK 7=DATA

///    VCDClass iec_export_vdc;      // Klasse zum Exportieren der IEC Signale
///    bool iec_is_dumped;

    /// Temporär ///
    bool        easy_flash_dirty;
    uint8_t     easy_flash_byte;

    PORT cia1_port_a;
    PORT cia1_port_b;
    PORT cia2_port_a;
    PORT cia2_port_b;

    bool        enable_mouse_1351;
    uint16_t    mouse_1351_x_rel;
    uint16_t    mouse_1351_y_rel;

    uint8_t     mouse_port;
    uint8_t     poti_ax;
    uint8_t     poti_ay;
    uint8_t     poti_bx;
    uint8_t     poti_by;

    uint8_t     poti_x;
    uint8_t     poti_y;

    uint8_t     key_matrix_to_port_a[8];
    uint8_t     key_matrix_to_port_b[8];

    bool return_key_is_down;

    bool reu_is_insert;

    /////////////////////// BREAKPOINTS ////////////////////////

    // Bit 0 = PC Adresse
    // Bit 1 = AC
    // Bit 2 = XR
    // Bit 3 = YR
    // Bit 4 = Lesen von einer Adresse
    // Bit 5 = Schreiben in einer Adresse
    // Bit 6 = Lesen eines Wertes
    // Bit 7 = Schreiben eines Wertes
    // Bit 8 = Beim erreichen einer bestommten Raster Zeile
    // Bit 9 = Beim erreichen eines Zyklus in einer Rasterzeile

///    uint16_t        breakpoints[0x10000];
///    uint16_t        break_values[16];
///    uint16_t        break_status;
///    bool            floppy_found_breakpoint;

///    uint8_t         breakgroup_count;
///    BREAK_GROUP     *breakgroup[MAX_BREAK_GROUP_NUM];

    ////////////////////////////////////////////////////////////

    bool        c64_reset_ready;
    bool        floppy_reset_ready[MAX_FLOPPY_NUM];

    string      c64_command_line;//[MAX_STRING_LENGTH];
    uint16_t    c64_command_line_lenght;
    uint16_t    c64_command_line_current_pos;
    bool        c64_command_line_status;
    bool        c64_command_line_count_s;

    uint32_t    cycle_counter;
///	int         limit_cycles_counter;        // Dieser Counter wird wenn er > 0 ist bei jeden Zyklus um 1 runtergezählt
	bool		hold_next_system_cycle;		 // Wird dieses Flag gesetzt wird verhindert das ein C64 Cylce ausgeführt wird

	bool        debug_mode;
    bool        debug_animation;
    float_t     animation_speed_add;
    float_t     animation_speed_counter;
    bool        one_cycle;
    bool        one_opcode;
    int         one_opcode_source;
    bool        cpu_states[5];              // true = Feetch / false = no Feetch ::: Index 0=C64 Cpu, 1-4 Floppy 1-4

    FILE*       debug_logging_file;
    bool        debug_logging;

    bool        warp_mode;

    bool        mouse_is_hidden;
    int         mouse_hide_counter;
    int         mouse_hide_time;

    bool        key_map_is_rec;
    uint8_t     rec_matrix_code;
};

#endif // C64CLASS_H
