cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)
include(pico_extras_import.cmake)
project(rp2040Term C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(RP2040TERM_VERSION_MAJOR "0")
set(RP2040TERM_VERSION_MINOR "2")
set(RP2040TERM_DESCR "Alpha-Beta" )

pico_sdk_init()

# Read version from file:
file(READ "Source/build_number.h" VERSION_STRING)
# Remove whitespace characters (like newline):
string(STRIP ${VERSION_STRING} VERSION_STRING)
# Match anything before build metadata (+ character) and save it as VER_NUM (usual regex would be: /^(.*?)\+/ but this is cmake):
string(REGEX MATCHALL "^(\.*)\\+" VER_NUM ${VERSION_STRING} )
# Match last number and save it as BUILD_NUM:
string(REGEX MATCHALL "([0-9]+)$" BUILD_NUM ${VERSION_STRING} )
# Increase build number:
math(EXPR BUILD_NUM "${BUILD_NUM}+1")
# Concatenate back to one string:
string(CONCAT VERSION_STRING ${VER_NUM} ${BUILD_NUM})
# Print info and save to file:
#message(NOTICE "New build number: ${VERSION_STRING}")
file(WRITE "Source/build_number.h" ${VERSION_STRING})

add_executable(rp2040Term
  source/main.c
  source/system/system.c
  source/conio/conio.c
  source/serial/serial.c 
  source/status/status.c
  source/keyboard/keyboard.c
  source/vt100/vt100.c
  source/fonts/font_sun8x16.c
  source/fonts/font_sans8x16.c
  source/fonts/font_serif8x16.c)

add_subdirectory(source/audio/RP2040-PWM-DMA-Audio-master)

target_compile_definitions(rp2040Term PUBLIC 
  PICO_SCANVIDEO_MAX_SCANLINE_BUFFER_WORDS=250
  PICO_SCANVIDEO_ENABLE_CLOCK_PIN
  PICO_SCANVIDEO_ENABLE_DEN_PIN
  TEXT_COLUMNS=60
  TEXT_COLUMNS_VISIBLE=60
  TEXT_ROWS=17
  TEXT_ROWS_VISIBLE=16
  STATUS_BAR_ROW=16
  POPUP_ROW=8
  CMAKE_PROJECT_VERSION="${RP2040TERM_VERSION_MAJOR}.${RP2040TERM_VERSION_MINOR}"
  CMAKE_PROJECT_DESCRIPTION="${RP2040TERM_DESCR}")

target_include_directories(rp2040Term PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/source
  ${CMAKE_CURRENT_LIST_DIR}/source/audio
  ${CMAKE_CURRENT_LIST_DIR}/source/conio
  ${CMAKE_CURRENT_LIST_DIR}/source/fonts
  ${CMAKE_CURRENT_LIST_DIR}/source/keyboard
  ${CMAKE_CURRENT_LIST_DIR}/source/serial
  ${CMAKE_CURRENT_LIST_DIR}/source/status
  ${CMAKE_CURRENT_LIST_DIR}/source/system
  ${CMAKE_CURRENT_LIST_DIR}/source/vt100)

pico_enable_stdio_uart(rp2040Term 0)
pico_enable_stdio_usb(rp2040Term 0)
pico_add_extra_outputs(rp2040Term)

target_link_libraries(rp2040Term PRIVATE
  pico_stdlib
  pico_scanvideo_dpi
  pwm_dma_audio
  pico_multicore
  hardware_pwm
  tinyusb_host
  tinyusb_board)

message("RP2040 Term 60Col TFT Version = ${RP2040TERM_VERSION_MAJOR}.${RP2040TERM_VERSION_MINOR}.${BUILD_NUM} ${RP2040TERM_DESCR}")
