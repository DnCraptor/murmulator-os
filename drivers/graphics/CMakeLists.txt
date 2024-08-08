add_library(graphics INTERFACE)

target_sources(graphics INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/graphics.c
    ${CMAKE_CURRENT_LIST_DIR}/font6x8.c
    ${CMAKE_CURRENT_LIST_DIR}/fnt8x16.c
)

#target_link_libraries(graphics INTERFACE
#vga-nextgen
#hdmi
#tft
#)

target_include_directories(graphics INTERFACE ${CMAKE_CURRENT_LIST_DIR})

#pico_generate_pio_header(st7789
#        ${CMAKE_CURRENT_LIST_DIR}/st7789.pio
#)
