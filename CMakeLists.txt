cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)
include(pico_extras_import.cmake)
project(colour_text_project C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set( RP2040TERM_VERSION 0.0.0.1)
set( RP2040TERM_DESCR "Alpha" )

pico_sdk_init()
add_executable(rp2040Term
  main.c
  conio.c
  font_sun8x16.c)

target_compile_definitions(rp2040Term PUBLIC 
  PICO_SCANVIDEO_MAX_SCANLINE_BUFFER_WORDS=250
  PICO_SCANVIDEO_ENABLE_CLOCK_PIN
  PICO_SCANVIDEO_ENABLE_DEN_PIN
  TEXT_COLUMNS=60
  TEXT_ROWS=17)

  # target_compile_options(colour_text PUBLIC -Werror -Wall -Wextra -Wnull-dereference)

target_include_directories(rp2040Term PRIVATE ${CMAKE_CURRENT_LIST_DIR})

pico_enable_stdio_uart(rp2040Term 0)
pico_enable_stdio_usb(rp2040Term 0)
pico_add_extra_outputs(rp2040Term)

target_link_libraries(rp2040Term PRIVATE
  pico_stdlib
  pico_scanvideo_dpi
  pico_multicore
  tinyusb_host
  tinyusb_board)

message("RP2040 Term 60Col TFT Version = ${RP2040TERM_VERSION} ${RP2040TERM_DESCR}")
