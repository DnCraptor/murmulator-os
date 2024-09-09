#pragma once

#include "m-os-api-cpp-string.h"
#include "m-os-api-cpp-vector.h"

struct _options
{
    string bootromfile;
    string romfile;
    int rom_org;
    int pc;
    string wavfile;
    vector<string> eddfile;
    int max_frame;
    bool vsync;
    bool novideo;
    int screen_width;
    int screen_height;
    int border_width;
    int center_offset;

    struct _volume {
        float timer;
        float beeper;
        float ay;
        float covox;
        float global;
    } volume;

    struct _enables {
        bool timer_ch0;
        bool timer_ch1;
        bool timer_ch2;
        bool ay_ch0;
        bool ay_ch1;
        bool ay_ch2;
    } enable;

    bool nofilter;      /* bypass audio filter */

    bool nosound;
    bool nofdc;
    bool bootpalette;

    bool autostart;
    bool window;        /* run in a window */
    int blendmode;      /* 0: no blend, 1: mix in doubled frames */

    bool opengl;
    struct _opengl_opts {
        /* e.g. myshader for myshader.vsh/fsh for use shader */
        string shader_basename; 
        bool use_shader;
        bool default_shader;
        bool filtering;
    } gl;

    struct _log {
        bool fdc;
        bool audio;
        bool video;
    } log;

    bool profile;       /* enable gperftools CPU profiler */

    bool vsync_enable;  /* true if window has mouse focus */

    vector<string> scriptfiles;
    vector<string> scriptargs;

    string path_for_frame(int n);
    vector<string> fddfile;
    vector<int> save_frames;
    string audio_rec_path;

    void load(const string & filename);
    void save(const string & filename);
    string get_config_path(void);
    void parse_log(const string & opt);
};

extern _options Options;

void options(int argc, char ** argv);
