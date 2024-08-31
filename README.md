# murmulator-os
Murmulator OS v.0.2.6.1<br/>

# Hardware needed
Raspberry Pi Pico (RP2040)<br/>
Sources are "in-progress" state and testing now only on Murmulator devboard with VGA/HDMI/TV(RGB) output.<br/>
Simplest Murmulator schema is availabele there: https://github.com/AlexEkb4ever/MURMULATOR_classical_scheme<br/>
![Murmulator Schematics](https://github.com/javavi/pico-infonesPlus/blob/main/assets/Murmulator-1_BSchem.JPG)
![VGA OR hdmi](./assets/vga_hdmi.jpg)

# This Archive
Extract MOS folder to your SD-Card to /MOS folder.

# Optional
There are several extra features supported, like external PSRAM and/or DAC, are availbel on more featured Murmulator versions.<br/>
Let use translate from russian on https://murmulator.ru/types site, for case you want to get more info.

# Current state
RP2040 core 0: starts FreeRTOS (based on https://github.com/FreeRTOS/FreeRTOS-Community-Supported-Demos/tree/3d475bddf7ac8af425da67cdaa2485e90a57a881/CORTEX_M0%2B_RP2040) <br/>
RP2040 core 1: starts VGA/HDMI/TV(RGB) driver (based on Murmulator comunity version last used before in https://github.com/xrip/pico-launcher)

# MOS build hints:
 - use SDK 1.5.1 https://github.com/raspberrypi/pico-setup-windows/releases<br/>
 - update TinyUSB in your SDK ("C:\Program Files\Raspberry Pi\Pico SDK v1.5.1\pico-sdk\lib\tinyusb") to version: cfbdc44a8d099240ad5ef208bd639487c2f28153    branch 'master' of https://github.com/hathach/tinyusb<br/>
 - update exit_from_boot2.S (patch/exit_from_boot2.S there) in SDK ("C:\Program Files\Raspberry Pi\Pico SDK v1.5.1\pico-sdk\src\rp2_common\boot_stage2\asminclude\boot2_helpers\exit_from_boot2.S")<br/>
 - cmake command (for CLI-build Win10/11): "C:\Program Files\Raspberry Pi\Pico SDK v1.5.1\cmake\bin\cmake.EXE" -DCMAKE_BUILD_TYPE:STRING=MinSizeRel -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE "-DCMAKE_C_COMPILER:FILEPATH=C:\Program Files\Raspberry Pi\Pico SDK v1.5.1\gcc-arm-none-eabi\bin\arm-none-eabi-gcc.exe" "-DCMAKE_CXX_COMPILER:FILEPATH=C:\Program Files\Raspberry Pi\Pico SDK v1.5.1\gcc-arm-none-eabi\bin\arm-none-eabi-g++.exe" --no-warn-unused-cli -SC:/Pico/murmulator/Cross/murmulator-os -Bc:/Pico/murmulator/Cross/murmulator-os/build -G Ninja<br/>
 - Linux: cmake -DCMAKE_BUILD_TYPE:STRING=MinSizeRel -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE "-DCMAKE_C_COMPILER:FILEPATH=/usr/b
in/arm-none-eabi-gcc" "-DCMAKE_CXX_COMPILER:FILEPATH=/usr/bin/arm-none-eabi-g++" --no-warn-unused-cli ... (see linux.build-alt.sh)<br/>
 - Linux: create symlinks:<br/>
  - /path/to/tinyusb-0.16.0/hw/bsp -> /path/to/murmulator-os/include/bsp<br/>
  - /path/to/tinyusb-0.16.0/hw/mcu -> /path/to/murmulator-os/include/mcu<br/>
 - for GUI VSCode build mode do not miss to set build Configuration to use "GCC 10.3.1. arm-none-eabi" and "MinSizeRel"<br/>

# MOS applications build hints:
 - use SDK 1.5.1 https://github.com/raspberrypi/pico-setup-windows/releases<br/>
 - ignore linker (ld) errors<br/>
 - for GUI VSCode build mode do not miss to set build Configuration to use "GCC 10.3.1. arm-none-eabi" and "Release" (or "MinSizeRel")<br/>
 - lookup for appropriated "obj" file, rename it and use as executable<br/>

# OS syscalls
TBA

# Murmulator OS tips
Murmulator OS (M-OS to make it short) has no specific levels of execution like kernel and application. All code parts (kernel/application) executes on the same level (like it was in good old DOS).<br/>
<br/>
M-OS uses a mono-kernel architecture, it means we have no separate kernel modules, all kernel related code is precompiled, prelinked and preinstalled on hardware. Only application level code is replaceable while kernel is working.<br/>
<br/>
M-OS is based on FreeRTOS port for RP2040 (Base version: FreeRTOSv202212.01. Examples used: https://github.com/FreeRTOS/FreeRTOS-Community-Supported-Demos/tree/3d475bddf7ac8af425da67cdaa2485e90a57a881/CORTEX_M0%2B_RP2040) which is running on core#0 of the RP2040. No SPI support for now. System time slice (tick) used by FreeRTOS = 1ms.<br/>
<br/>
M-OS manages access to<br/>
 - SRAM (256+4+4KB installed on RP2040),<br/>
 - PSRAM (4-8MB installed on Murmulator board since v.1.3),<br/>
 - FLASH ROM (2-16MB installed on Rasperry Pi Pico),<br/>
 - SD-CARD (FAT32, connected to Murmulator board),<br/>
 - VGA/HDMI/TV adapter (222-color schema, installed on Murmulator board),<br/>
 - PS/2 keyboard (connected to Murmulator board),<br/>
 - Kempston (Dendy 8-bit) Joystick (connected to Murmulator board),<br/>
 - Sound devices (PWM 12-bit stereo + "speaker" or TDA1387T/PCM5102A DAC chip in "PCM" build versions)<br/>
<br/>
Other types of<br/>
 - video outputs: TFT/ILI9341/...;<br/>
 - inputs: USB HUD devices, PS/2 and other mouse types, Wii Joystick...;<br/>
to be supported later.<br/>
<br/>
M-OS is installed as bootable application for RP2040 on Rasperry Pi Pico FLASH ROM.<br/>
Addresses map (16MB flash):<br/>
10000000..10001000h - .boot2       - contains 4k startup block<br/>
10001000..10FE0000h - 4063 M-OS managed 4k pages, that may be provided to or shared with application level<br/>
10FE0000..10FFF000h - .text        - contains 124k of M-OS core<br/>
10FFF000..11000000h - .sys_table   - contains 4k table of pointers to functions, M-OS provides for application level<br/> 
(memmap.ld to be provided)<br/>
<br/>
M-OS application should try to reuse functions from OS instead of linking other libraries.<br/>
M-OS application should not use malloc/calloc/free functions from standard library, but call specific wrappers provided in m-os-api.h (TBA - full list of functions)<br/>
<br/>
Example of application level wrapper definition (will be provided whole set of wrappers in the m-os-api.h header file):<br/>

#define M_OS_API_SYA_TABLE_BASE (void*)0x10001000ul<br/>
static const unsigned long * const _sys_table_ptrs = M_OS_API_SYA_TABLE_BASE;<br/>
typedef void (*draw_text_ptr_t)(const char *string, uint32_t x, uint32_t y, uint8_t color, uint8_t bgcolor);<br/>
#define _draw_text_ptr_idx 25<br/>
#define draw_text ((draw_text_ptr_t)_sys_table_ptrs[_draw_text_ptr_idx])<br/>
<br/>
So it will be possible to call draw_text the same way as for case graphics.h and VGA/HDMI/TV driver exists, but without it.<br/>

# M-OS commands
cls - clear screen<br/>
dir / ls [dir] - show directory content. Use Ctrl+C to interrupt.<br/>
rm / del / era [file] - remove file (or empty directory)<br/>
cd [dir] - change current directory<br/>
cp [file1] [file2] - copy file1 as file2<br/>
mkdir [dir] - create directory<br/>
cat / type [file] - type file. Use Ctrl+C to interrupt.<br/>
rmdir [dir] - remove directory (recurive)<br/>
elfinfo [file] - provide .elf file info<br/>
psram - provide some psram info. Use Ctrl+C to interrupt.<br/>
swap - provide some swap info. Use Ctrl+C to interrupt.<br/>
sram - reference speed of swap base SRAM. Use Ctrl+C to interrupt.<br/>
cpu - show current CPU freq. and dividers, `cpu [NNN]` - change freq. to NNN MHz (it may hang on such action)<br/>
mem - show current memory state<br/>
set - show or set environment variables<br/>
export - put variable into system context<br/>
mode [#] - set video-mode, for now it is supported:<br/>
<li>
 <ul>0 - 53x30, 1 - 80x30, 2 - 100x37, 3 - 128x48, 4 - 256x256x2-bit, 5 - 512x256x1-bit, 6 - 320x240x4-bit, 7 - 320x240x8-bit, 8 - 640x480x4-bit - for VGA</ul>
 <ul>0 - 53x30 and 1 - 80x30, 2 - 320x240x4-bit for HDMI</ul>
 <ul>0 - 53x30 for TV (RGB)</ul>
</li>
less - show not more than one page of other command in pipe, like `ls | less`. Use Ctrl+C for exit.<br/>
hex [file]/[@addr] - show file or RAM as hexidecimal dump. Use Ctrl+C to interrupt.<br/>
tail [-n #] [file] - show specified (or 10) last lines from the file. Use Ctrl+C to interrupt.<br/>
usb [on/off] - start a process to mount murmulator CD-card as USB-drive (NESPAD [B] button in mc)<br/>
mc - Murmulator Commander, use [CTRL]+[O] to show console, and [CTRL]+[Enter] for fast type current file path<br/>
mcview [file] - Murmulator Commander Viewer<br/>
mcedit [file] - Murmulator Commander Editor<br/>
mv [from_file_name] [to_file_name] - move/rename the file<br/>
gmode [#] - simple graphics mode test<br/>
font [width] [height] - show/set font size for graphics modes, like `font 6 8`<br/>
blimp [n1] [n2] - simple sound test. [n1] number of cycles, [n2] OS ticks between high and low levels (1/freq.)<br/>
wav [file] - simple .wav files player (tested on 8 kHz 1-channel 16-bit files). Use Ctrl+C to interrupt.<br/>
basic [file] - tiny basic interperator implementation (by Stefan Lenz, see https://github.com/slviajero/tinybasic for more info). Use Ctrl+C to interrupt.<br/>
ps - list of "processes" (FreeRTOS tasks).<br/>
kill [n] - send SIGKILL to a "process" (FreeRTOS task), [n] - task number returned by the `ps` utility.<br/>
dhrystone [n] [kHz] - small performance test (see: https://github.com/DnCraptor/arm_benchmarks)<br/>
whetstone [n] - MIPS (whetstone) performance test (see: https://github.com/DnCraptor/arm_benchmarks)<br/>
<br/>
[cmd] &gt; [file] - output redirection to file<br/>
[ENTER] - start command / flash and run .uf2 file in "demo" format (NESPAD [A] button in mc)<br/>
[TAB] - autocomplete<br/>
[BACKSPACE] - remove last character<br/>
[CTRL]+[SHIFT] - rus/lat<br/>
[CTRL]+[ALT]+[DEL] - reset (NESPAD [SELECT]+[B])<br/>
[CTRL]+[TAB]+[+] - increase CPU freq.</br>
[CTRL]+[TAB]+[-] - decrease CPU freq.</br>
[ALT]+[0-9]+[0-9]+[0-9] - manual enter some character by its decimal code (CP-866 codepage)<br/>

# M-OS system variables
BASE - base directory with commands implementations<br/>
SWAP - swap settings<br/>
COMSPEC - a path to command interpretator<br/>
PATH - list of directories to lookup for applications<br/>
GMODE - set initial graphics mode<br/>
TEMP - specify a folder with temporary files<br/>

# Boot-loader mode
If uf2 application was started from M-OS, and such application is not designed for M-OS, it is possible to return to M-OS only via reboot:<br/>
Press [F11] or [SPACE] (DPAD [SELECT]) and hold on the Murmulator reset or power-on, in this case uf2 application startup will be skip and you will return to the M-OS. And in case DPAD [B] button is also hold, it automounts SD-Cart as USB-drive.<br/>
Press [F12] or [ENTER] (DPAD [START]) and hold on the Murmulator reset or power-on, in case you want to start USB-drive mode prior starting M-OS.<br/>
Press [TAB] (DPAD [A]) and hold on the Murmulator reset or power-on, in case you want to replace default output by seconday driver.<br/>
Press DPAD [B] and hold on the Murmulator reset or power-on, in case you want to switch default output to third output.<br/>
Press [HOME] and hold on the Murmulator reset or power-on, in case you want  start USB-drive mode prior starting M-OS.<br/>
