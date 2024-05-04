# murmulator-os
ZX Murmulator OS<br/>

# Hardware needed
Raspberry Pi Pico (RP2040)<br/>
Sources are "in-progress" state and testing now only on ZX Murmulator devboard with VGA output.<br/>
Simplest Murmulator schema is availabele there: https://github.com/AlexEkb4ever/MURMULATOR_classical_scheme<br/>
![Murmulator Schematics](https://github.com/javavi/pico-infonesPlus/blob/main/assets/Murmulator-1_BSchem.JPG)

# Optional
There are several features to be supported, like external PSRAM and DAC, are availbel on more featured murmulator versions.<br/>
Let use translate from russian on site https://murmulator.ru/types, for case it is required for you.

# Current state
RP2040 core 0: starts FreeRTOS (based on https://github.com/FreeRTOS/FreeRTOS-Community-Supported-Demos/tree/3d475bddf7ac8af425da67cdaa2485e90a57a881/CORTEX_M0%2B_RP2040) <br/>
RP2040 core 1: starts VGA driver (based on ZX Murmulator comunity version last used before it in https://github.com/xrip/pico-launcher)

# OS syscalls
TBA

# ZX Murmulator OS tips
ZX Murmulator OS (M-OS to make it short) has no specific levels of execution like kernel and application. All code parts (kernel/application) executes on the same level (like it was in good old DOS).<br/>
<br/>
M-OS uses a micro-kernel architecture, it means we have no separate kernel modules, all kernel related code is precompiled, prelinked and preinstalled on hardware. Only application level code is replaceable while kernel is working.<br/>
<br/>
M-OS is based on FreeRTOS port for RP2040 (Base version: FreeRTOSv202212.01. Examples used: https://github.com/FreeRTOS/FreeRTOS-Community-Supported-Demos/tree/3d475bddf7ac8af425da67cdaa2485e90a57a881/CORTEX_M0%2B_RP2040) which is running on core#0 of the RP2040. No SPI support for now. Time slice used by FreeRTOS = 1ms.<br/>
<br/>
M-OS manages access to<br/>
 - SRAM (256+4+4KB installed on RP2040),<br/>
 - PSRAM (4-8MB installed on ZX Murmulator board since v.1.3),<br/>
 - FLASH ROM (2-16MB installed on Rasperry Pi Pico),<br/>
 - SD-CARD (FAT32, connected to ZX Murmulator board),<br/>
 - VGA adapter (222-color schema, installed on ZX Murmulator board),<br/>
 - PS/2 keyboard (connected to ZX Murmulator board),<br/>
 - Kempston (Dendy 8-bit) Joystick (connected to ZX Murmulator board),<br/>
 - Sound devices (PWM 12-bit stereo + "speaker")<br/>
<br/>
Other types of<br/>
 - video outputs: HDMI/TV/...;<br/>
 - inputs: USB HUD devices, PS/2 and other mouse types, Wii Joystick...;<br/>
 - sound outputs: TDA...<br/>
to be supported later.<br/>
<br/>
M-OS is installed as bootable application for RP2040 on Rasperry Pi Pico FLASH ROM.<br/>
Addresses map:<br/>
10000000..10001000h - .boot2       - contains startup block<br/>
10001000..10002000h - .sys_table   - contains a table of pointers to functions, M-OS provides for application level<br/>
10002000..10003000h - .boota       - contains a table of pointers to functions, application provides for M-OS (for now only launcher)<br/>
10003000..(depends on FLASH size)  - M-OS managed place, that may be provided to or shared with application level<br/>
(memmap.ld to be provided)<br/>
<br/>
M-OS application should try to reuse functions from OS instead of linking other libraries.<br/>
M-OS application should not use malloc/calloc/free functions from standard library, but call specific wrappers provided in m-os-api.h (TBA - full list of functions)<br/>
<br/>
M-OS application may use static RAM variables/arrays, but limited to specific region, that is not used by OS. (TBA - start address 20020000?, memmap.ld to be provided)<br/>
<br/>
Example of application level wrapper definition (will be provided whole set of wrappers in the m-os-api.h header file):<br/>

#define M_OS_API_SYA_TABLE_BASE (void*)0x10001000ul<br/>
static const unsigned long * const _sys_table_ptrs = M_OS_API_SYA_TABLE_BASE;<br/>
typedef void (*draw_text_ptr_t)(const char *string, uint32_t x, uint32_t y, uint8_t color, uint8_t bgcolor);<br/>
#define _draw_text_ptr_idx 25<br/>
#define draw_text ((draw_text_ptr_t)_sys_table_ptrs[_draw_text_ptr_idx])<br/>
<br/>
So it will be possible to call draw_text the same way as for case graphics.h and VGA driver exists, but without 'em.<br/>

# M-OS commands
cls - clear screen<br/>
dir / ls [dir] - show directory content<br/>
rm / del / era file - remove file<br/>
cd dir - change current directory<br/>
cp file1 file2 - copy file1 as file2<br/>
mkdir dir - create directory<br/>
cat / type file - type file<br/>
[cmd] &gt; file - output redirection to file<br/>
[ENTER] - start command<br/>
[TAB] - autocomplete<br/>
[BACKSPACE] - remove last character<br/>
