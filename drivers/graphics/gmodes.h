#pragma once

#ifndef GMODES_H
#define GMODES_H

typedef enum graphics_mode_t {
    TEXTMODE_DEFAULT = 0,
    TEXTMODE_53x30 = 0, // 640*480 (320*240)
    TEXTMODE_80x30 = 1, // 640*480
    TEXTMODE_100x37 = 2, // 800*600
    TEXTMODE_128x48 = 3, // 1024*768
    GRAPHICSMODE_DEFAULT = 4, // 640*480 (320*240)
    VGA_320x240x256 = 4, // 640*480 (320*240)
    BK_256x256x2 = 5, // 1024*768 3-lines->1 4-pixels->1
    BK_512x256x1 = 6, // 1024*768 3-lines->1 2-pixels->1
} graphics_mode_t;

#endif

