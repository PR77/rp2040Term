/*  main.c - The main program -------------------------------------------------
__________ __________      _________________________________
\______   \\______   \     \__    ___/\_   _____/\__    ___/
 |       _/ |     ___/       |    |    |    __)    |    |   
 |    |   \ |    |           |    |    |     \     |    |   
 |____|_  / |____|     /\    |____|    \___  /     |____|   
        \/             \/                  \/               
https://patorjk.com/software/taag/#p=display&h=0&v=0&f=Graffiti

RP2040 Terminal Emulator for parallel RBG666 TFT Display using the RP2040
scanvideo library. I used bits-and-pieces for these awesome repos to develop
this;

https://geoffg.net/terminal.html
https://github.com/Memotech-Bill/pico-colour-text/tree/main
https://github.com/RC2014Z80/picoterm

-----------------------------------------------------------------------------*/

#include "pico.h"
#include "pico/stdlib.h"
#include "pico/scanvideo.h"
#include "pico/scanvideo/composable_scanline.h"
#include "pico/multicore.h"
#include "tusb.h"
#include "class/hid/hid.h"
#include "main.h"
#include "conio.h"
#include "serial.h"
#include "status.h"
#include "keyboard.h"
#include "system.h"
#include "build_number.h"

static semaphore_t videoInitialised;

void system_initialiseScanVideo(void) {

    scanvideo_setup(&tftLQ042_480x272_60);
    scanvideo_timing_enable(true);
    sem_release(&videoInitialised);
    system_renderLoop();
}

int main(void) {

    uint8_t msgBuffer[TEXT_COLUMNS_VISIBLE];

    /* PLL Calculations --> python vcocalc.py 123.5
    Requested: 123.5 MHz
    Achieved: 123.5 MHz
    REFDIV: 2
    FBDIV: 247 (VCO = 1482.0 MHz)
    PD1: 6
    PD2: 2
    */
    set_sys_clock_pll(1482000000, 6, 2);

    system_initialiseSystem();
    conio_initialiseCharacterBuffer(PALETTE_COLOUR_AMBER_INDEX, PALETTE_COLOUR_BLACK_INDEX);
    status_initialiseStatusBar(PALETTE_COLOUR_GREEN_INDEX, PALETTE_COLOUR_BLACK_INDEX, true);

    serial_initialiseTerminalUart(uart1);
    serial_attachReceivedCharacterHandler(&conio_printSimpleCharacter);

    keyboard_initialiseKeyboard();
    keyboard_attachSystemResetHandler(&system_executeSystemReset);
    keyboard_attachCustomKeyHandler(HID_KEY_F1, &conio_clearScreenHomeCursor, NULL);
    keyboard_attachCustomKeyHandler(HID_KEY_F2, &system_cycleDisplayFont, NULL);
    keyboard_attachCustomKeyHandler(HID_KEY_F8, &system_toggleBeeper, NULL);
    keyboard_attachCustomKeyHandler(HID_KEY_F9, &system_toggleCRLF, NULL);
    keyboard_attachCustomKeyHandler(HID_KEY_F10, &system_decreaseBacklightByStep, NULL);
    keyboard_attachCustomKeyHandler(HID_KEY_F11, &system_toggleLocalEcho, NULL);
    keyboard_attachCustomKeyHandler(HID_KEY_F12, &system_increaseBacklightByStep, NULL);
    keyboard_attachDefaultKeyHandler(&system_handleKeyboardAndUartTransmitRouting, NULL);
    
    // Create a semaphore to be posted when video init is complete
    sem_init(&videoInitialised, 0, 1);
    multicore_launch_core1(system_initialiseScanVideo);
    // wait for initialization of video to be complete
    sem_acquire_blocking(&videoInitialised);

    // Audio playback needs to be initialised after scanvideo due to DMA assignments.
    // Scanvideo requires DMA Channels 0 + 1, and will fail if not available.
    system_initialiseAudioPlayer();

	//                 012345678901234567890123456789012345678901234567890123456789
	//                 |        |         |         |         |         |         |
    conio_printString("--------------------------------[RP2040Term TFT - By PR77]--", PALETTE_COLOUR_YELLOW_INDEX, PALETTE_COLOUR_BLACK_INDEX);
    conio_printString("__________ __________      _________________________________", PALETTE_COLOUR_AMBER_INDEX, PALETTE_COLOUR_BLACK_INDEX);
    conio_printString("\\______   \\\\______   \\     \\__    ___/\\_   _____/\\__    ___/", PALETTE_COLOUR_AMBER_INDEX, PALETTE_COLOUR_BLACK_INDEX);
    conio_printString(" |       _/ |     ___/       |    |    |    __)    |    |\r\n", PALETTE_COLOUR_AMBER_INDEX, PALETTE_COLOUR_BLACK_INDEX);   
    conio_printString(" |    |   \\ |    |           |    |    |     \\     |    |\r\n", PALETTE_COLOUR_AMBER_INDEX, PALETTE_COLOUR_BLACK_INDEX);   
    conio_printString(" |____|_  / |____|     /\\    |____|    \\___  /     |____|\r\n", PALETTE_COLOUR_AMBER_INDEX, PALETTE_COLOUR_BLACK_INDEX);   
    conio_printString("        \\/             \\/                  \\/\r\n", PALETTE_COLOUR_AMBER_INDEX, PALETTE_COLOUR_BLACK_INDEX);               

	//                 012345678901234567890123456789012345678901234567890123456789
	//                 |        |         |         |         |         |         |
    conio_printString("--[GPL 3 Clause]--------------------------------[(c) 2024]--", PALETTE_COLOUR_YELLOW_INDEX, PALETTE_COLOUR_BLACK_INDEX);
    conio_printString("\r\n", PALETTE_COLOUR_WHITE_INDEX, PALETTE_COLOUR_BLACK_INDEX);

    //                 012345678901234567890123456789012345678901234567890123456789
	//                 |        |         |         |         |         |         |
    snprintf(msgBuffer, TEXT_COLUMNS_VISIBLE, "Version %s, Build %d, Release %s\r\n", CMAKE_PROJECT_VERSION, BUILD_NUMBER, CMAKE_PROJECT_DESCRIPTION);
    conio_printString(msgBuffer, PALETTE_COLOUR_YELLOW_INDEX, PALETTE_COLOUR_BLACK_INDEX);
    conio_enableCursor();

    while (true) {
        system_updateLedTask();
        conio_updateCursorTask();
        status_updateStatusBarTask();
        keyboard_updateKeyboardTask();
        system_updateBellTask();
    }
}
