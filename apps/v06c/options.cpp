#include "m-os-api-cpp-string.h"

#define DEFAULT_BORDER_WIDTH 10
#define DEFAULT_SCREEN_WIDTH (512 + 2*(DEFAULT_BORDER_WIDTH))
#define DEFAULT_SCREEN_HEIGHT (256 + 16 + 16)
#define DEFAULT_CENTER_OFFSET (152 - DEFAULT_BORDER_WIDTH)

_options Options = 
{
    .bootromfile = "",
    .romfile = "",
    .rom_org = 256,
    .pc = 0,
    .wavfile = "",
    .eddfile = {},
    .max_frame = -1,
    .vsync = false,
    .novideo = false,
    .screen_width = DEFAULT_SCREEN_WIDTH,
    .screen_height = DEFAULT_SCREEN_HEIGHT,
    .border_width = DEFAULT_BORDER_WIDTH,
    .center_offset = DEFAULT_CENTER_OFFSET,

    // timer, beeper, ay, covox, global
    .volume = {0.1f, 0.1f, 0.1f, 0.1f, 1.5f},
    .enable = {/* timer ch0..2 */ true, true, true,
               /* ay ch0..2 */    true, true, true},
    .nofilter = false,
};

string _options::path_for_frame(int n)
{
    string res;
    res += "frame";
    res += n;
    return res;
}
