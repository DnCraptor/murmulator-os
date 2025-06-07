cmake_minimum_required(VERSION 3.22)
include(pico_sdk_import.cmake)

# Pull in FreeRTOS
set( FREERTOS_KERNEL_PATH "../Source" )
include(FreeRTOS_Kernel_import.cmake)

project(murmulator-os C CXX ASM)
pico_sdk_init()

option(HDMI "Enable HDMI display" OFF)
option(VGA "Enable VGA" OFF)
option(TFT "Enable TFT display" OFF)
option(ILI9341 "Enable TFT ILI9341 display" OFF)
option(FLASH_SIZE "Target Flash Size" 16384)
option(ZERO "RP2040-PiZero" OFF)
option(PICO_PC "RP2040-PICO-PC" OFF)

#set(VGA ON)
set(HDMI ON)
#set(ZERO ON)
set(PICO_PC ON)

if(NOT FLASH_SIZE)
#set(FLASH_SIZE 2048)
set(FLASH_SIZE 16384)
endif ()

set(OS_FLASH_SIZE 128)

set(MOS_VERSION "0.3.5")

if (ZERO)
    SET(BUILD_NAME "z0p1-${PROJECT_NAME}-${MOS_VERSION}")
elseif (PICO_PC)
    SET(BUILD_NAME "PCp1-${PROJECT_NAME}-${MOS_VERSION}")
else()
    SET(BUILD_NAME "m1p1-${PROJECT_NAME}-${MOS_VERSION}")
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(OUTPUT_DIR "${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${OUTPUT_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${OUTPUT_DIR}")

set(FAMILY rp2040)
set(BOARD pico_sdk)

add_subdirectory(drivers/audio)
add_subdirectory(drivers/ps2)
add_subdirectory(drivers/fatfs)
add_subdirectory(drivers/sdcard)
add_subdirectory(drivers/nespad)
add_subdirectory(drivers/psram)

add_subdirectory(drivers/vga-nextgen)
#add_subdirectory(drivers/st7789)
add_subdirectory(drivers/hdmi)
add_subdirectory(drivers/tv)
add_subdirectory(drivers/tv-software)
add_subdirectory(drivers/graphics)

# INCLUDE FILES THAT SHOULD BE COMPILED:
file(GLOB_RECURSE SRC "src/*.cpp" "src/*.c" "drivers/usb/*.c")
#  "Minimal/*.c" ?

message(STATUS "Add source files:")
foreach (SRC_FILE IN LISTS SRC)
    message(STATUS "${SRC_FILE}")
endforeach ()
message(STATUS "")

add_executable(${PROJECT_NAME} ${SRC})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/api
    ${CMAKE_CURRENT_SOURCE_DIR}/drivers/usb
)

configure_file(memmap.ld.in memmap.ld @ONLY)
pico_set_linker_script(${PROJECT_NAME} ${CMAKE_CURRENT_BINARY_DIR}/memmap.ld)

pico_set_program_name(${PROJECT_NAME} "Murmulator OS")
pico_set_program_version(${PROJECT_NAME} "${MOS_VERSION}")

target_link_libraries(${PROJECT_NAME} PRIVATE
        psram
        ps2
        sdcard
        fatfs
        nespad
        audio

        graphics

        pico_runtime
        pico_stdlib
        pico_rand
        pico_multicore
        hardware_dma
        hardware_pio
        hardware_i2c
        hardware_interp
        hardware_timer
        hardware_clocks
        hardware_pwm
        hardware_flash

        tinyusb_board
        tinyusb_device
        FreeRTOS-Kernel
        FreeRTOS-Kernel-Heap4
        
)

#family_configure_device_example(${PROJECT_NAME} noos)
target_link_options(${PROJECT_NAME} PRIVATE -Xlinker --print-memory-usage)
target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/drivers/usb
)

if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
target_compile_options(${PROJECT_NAME} PUBLIC
        -Wno-error=suggest-attribute=format
        -Wno-error=cast-qual
        -Wno-error=unused-parameter
        -Wno-error=conversion
        -Wno-error=format=
        -Wno-error=sign-compare
        -Wno-error=missing-field-initializers
        -Wno-error=switch
        -Wno-error=implicit-fallthrough=
        -Wno-error=stringop-truncation
        -Wno-error=restrict
        -w
)
endif ()

if (ZERO)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        OVERCLOCKING=252
        PICO_FLASH_SIZE_BYTES=16777216
        ZERO=1

        # do not touch last 512 bytes in RAM on launch app, say, no more RAM
        RESERVED_RAM=512

        # VGA 8 pins starts from pin:
        VGA_BASE_PIN=22

        # HDMI 8 pins starts from pin:
        HDMI_BASE_PIN=22

        # TFT
        TFT_CS_PIN=22
        TFT_RST_PIN=24
        TFT_LED_PIN=25
        TFT_DC_PIN=26
        TFT_DATA_PIN=28
        TFT_CLK_PIN=29

        # SDCARD
        SDCARD_PIN_SPI0_SCK=18
        SDCARD_PIN_SPI0_MOSI=19
        SDCARD_PIN_SPI0_MISO=20
        SDCARD_PIN_SPI0_CS=21

        # PS2 keyboard
        KBD_CLOCK_PIN=0
        KBD_DATA_PIN=1

        PICO_DEFAULT_LED_PIN=4

        # NES Gamepad
        NES_GPIO_CLK=7
        NES_GPIO_LAT=8
        NES_GPIO_DATA=9
        NES_GPIO_DATA2=10

       # PSRAM
        # PSRAM_MUTEX=1
        PSRAM_SPINLOCK=1
        PSRAM_ASYNC=1

        PSRAM_PIN_CS=2
        PSRAM_PIN_SCK=3
        PSRAM_PIN_MOSI=4
        PSRAM_PIN_MISO=5

        WAV_IN_PIO=14

        #I2S_SOUND

        BEEPER_PIN=11
        PWM_PIN0=12
        PWM_PIN1=13

        #BOOT_DEBUG_ACC=0
        #DEBUG_HEAP_SIZE=256
        #DEBUG_VGA=1
        #DEBUG_APP_LOAD=1
        #CFG_TUSB_DEBUG=3

        M_API_VERSION=25
        MIN_API_VERSION=14
        MOS_VERSION_STR="${MOS_VERSION}"
        #DEFAULT_VIDEO_DRIVER=SOFTTV_DRV
        #DEFAULT_VIDEO_DRIVER=RGB_DRV

        TESTS=0
    )
ELSEIF(PICO_PC)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        OVERCLOCKING=378
        PICO_FLASH_SIZE_BYTES=16777216

        # do not touch last 512 bytes in RAM on launch app, say, no more RAM
        RESERVED_RAM=512
      
        PICO_PC=1
        # GP-5  UXT1-10
        # GP-8  UXT1-6
        # GP-9  UXT1-5
        # GP-21 UXT1-4
        # GP-20 UXT1-3

        # QWST1 3/4 - GP2/3

        # DBG1 1/2
        KBD_CLOCK_PIN=0
        KBD_DATA_PIN=1

        WAV_IN_PIO=5 # UXT1-10

        #I2S_SOUND

        BEEPER_PIN=26
        PWM_PIN0=27
        PWM_PIN1=28

        # SDCARD
        SDCARD_PIN_SPI0_MISO=4
        SDCARD_PIN_SPI0_SCK=6
        SDCARD_PIN_SPI0_MOSI=7
        SDCARD_PIN_SPI0_CS=22

        #PSRAM           # not implemented
        PSRAM_SPINLOCK=1 # not implemented
        PSRAM_ASYNC=1    # not implemented

        PSRAM_PIN_CS=3   # not implemented
        PSRAM_PIN_SCK=3  # not implemented
        PSRAM_PIN_MOSI=3 # not implemented
        PSRAM_PIN_MISO=3 # not implemented

        # NES Gamepad
        USE_NESPAD
        NES_GPIO_CLK=8    # UXT1-6
        NES_GPIO_LAT=9    # UXT1-5
        NES_GPIO_DATA=20  # UXT1-3
        NES_GPIO_DATA2=21 # UXT1-4

        # VGA 8 pins starts from pin:
        VGA_BASE_PIN=12

        # HDMI 8 pins starts from pin:
        HDMI_BASE_PIN=12

        M_API_VERSION=25
        MIN_API_VERSION=14
        MOS_VERSION_STR="${MOS_VERSION}"

        TESTS=0
    )
else ()
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        OVERCLOCKING=378
        PICO_FLASH_SIZE_BYTES=16777216

        # do not touch last 512 bytes in RAM on launch app, say, no more RAM
        RESERVED_RAM=512

        # VGA 8 pins starts from pin:
        VGA_BASE_PIN=6

        # HDMI 8 pins starts from pin:
        HDMI_BASE_PIN=6

        # TFT
        TFT_CS_PIN=6
        TFT_RST_PIN=8
        TFT_LED_PIN=9
        TFT_DC_PIN=10
        TFT_DATA_PIN=12
        TFT_CLK_PIN=13

        # SDCARD
        SDCARD_PIN_SPI0_SCK=2
        SDCARD_PIN_SPI0_MOSI=3
        SDCARD_PIN_SPI0_MISO=4
        SDCARD_PIN_SPI0_CS=5

        # PS2 keyboard
        KBD_CLOCK_PIN=0
        KBD_DATA_PIN=1

        # NES Gamepad
        NES_GPIO_CLK=14
        NES_GPIO_DATA=16
        NES_GPIO_LAT=15

        PSRAM
        # PSRAM_MUTEX=1
        PSRAM_SPINLOCK=1
        PSRAM_ASYNC=1

        PSRAM_PIN_CS=18
        PSRAM_PIN_SCK=19
        PSRAM_PIN_MOSI=20
        PSRAM_PIN_MISO=21

        WAV_IN_PIO=22

        #I2S_SOUND

        PWM_PIN0=26
        PWM_PIN1=27
        BEEPER_PIN=28

        #BOOT_DEBUG_ACC=0
        #DEBUG_HEAP_SIZE=256
        #DEBUG_VGA=1
        #DEBUG_APP_LOAD=1
        #CFG_TUSB_DEBUG=3

        M_API_VERSION=25
        MIN_API_VERSION=14
        MOS_VERSION_STR="${MOS_VERSION}"
        #DEFAULT_VIDEO_DRIVER=SOFTTV_DRV
        #DEFAULT_VIDEO_DRIVER=RGB_DRV

        TESTS=0
    )
endif ()

if (HDMI)
    target_compile_definitions(${PROJECT_NAME} PRIVATE DEFAULT_VIDEO_DRIVER=HDMI_DRV)
elseif(VGA)
    target_compile_definitions(${PROJECT_NAME} PRIVATE DEFAULT_VIDEO_DRIVER=VGA_DRV)
else()
    target_compile_definitions(${PROJECT_NAME} PRIVATE DEFAULT_VIDEO_DRIVER=RGB_DRV)
endif()

pico_define_boot_stage2(slower_boot2 ${PICO_DEFAULT_BOOT_STAGE2_FILE})
math(EXPR CUSTOM_XIP_BASE "1024 * (${FLASH_SIZE} - ${OS_FLASH_SIZE}) + 0x10000000")
message(STATUS "${CUSTOM_XIP_BASE}")
target_compile_definitions(slower_boot2 PRIVATE CUSTOM_XIP_BASE=${CUSTOM_XIP_BASE})
target_compile_definitions(slower_boot2 PRIVATE PICO_FLASH_SPI_CLKDIV=4)
pico_set_boot_stage2(${PROJECT_NAME} slower_boot2)


target_compile_definitions(${PROJECT_NAME} PRIVATE
    FLASH_SIZE=${FLASH_SIZE}
    OS_FLASH_SIZE=${OS_FLASH_SIZE}
)

IF (TFT)
    target_link_libraries(${PROJECT_NAME} PRIVATE st7789)
    target_compile_definitions(${PROJECT_NAME} PRIVATE TFT)
    SET(BUILD_NAME "${BUILD_NAME}-TFT")
    IF (ILI9341)
        SET(BUILD_NAME "${BUILD_NAME}-ILI9341")
        target_compile_definitions(${PROJECT_NAME} PRIVATE ILI9341)
    ELSE ()
        SET(BUILD_NAME "${BUILD_NAME}-ST7789")
    ENDIF ()
ELSE ()
    target_compile_definitions(${PROJECT_NAME} PRIVATE VGA)
    target_link_libraries(${PROJECT_NAME} PRIVATE vga-nextgen)
    target_compile_definitions(${PROJECT_NAME} PRIVATE HDMI)
    target_link_libraries(${PROJECT_NAME} PRIVATE hdmi)
    target_compile_definitions(${PROJECT_NAME} PRIVATE TV)
    target_link_libraries(${PROJECT_NAME} PRIVATE tv)
   # target_compile_definitions(${PROJECT_NAME} PRIVATE SOFTTV)
   # target_link_libraries(${PROJECT_NAME} PRIVATE tv-software)
    if (HDMI)
        SET(BUILD_NAME "${BUILD_NAME}-HDMI-VGA-RGB")
    elseif(VGA)
        SET(BUILD_NAME "${BUILD_NAME}-VGA-HDMI-RGB")
    else()
        SET(BUILD_NAME "${BUILD_NAME}-RGB-VGA-HDMI")
    endif()
ENDIF ()

set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "${BUILD_NAME}")

pico_enable_stdio_uart(${PROJECT_NAME} 0)
pico_enable_stdio_usb(${PROJECT_NAME} 0)

#pico_set_binary_type(${PROJECT_NAME} no_flash)
#pico_set_binary_type(${PROJECT_NAME} copy_to_ram)

pico_add_extra_outputs(${PROJECT_NAME})
