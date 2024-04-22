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
