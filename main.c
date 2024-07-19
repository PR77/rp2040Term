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
#include "hardware/clocks.h"
#include "hardware/irq.h"
#include "hardware/pwm.h"
#include "time.h"
#include "bsp/board.h"
#include "tusb.h"
#include "class/hid/hid.h"
#include "font_sun8x16.h"
#include "main.h"
#include "conio.h"
#include "serial.h"
#include "status.h"
#include "keyboard.h"
#include "build_number.h"

#define VID_CORE                1
#define LCD_BACKLIGHT_PWM_PIN   28

static semaphore_t videoInitialised;
static st_systemConfiguration systemConfiguration;

void __time_critical_func(renderLoop)(void) {
    
    int core_num = get_core_num();
    assert (core_num >= 0 && core_num < 2);

    while (true)
    {
        struct scanvideo_scanline_buffer *buffer = scanvideo_begin_scanline_generation(true);
        uint16_t *pix = (uint16_t *)buffer->data;
        uint16_t scanlineNumber = scanvideo_scanline_number(buffer->scanline_id);
        uint8_t rowIndex = (scanlineNumber / GLYPH_HEIGHT);
        uint8_t rowScanline = (scanlineNumber % GLYPH_HEIGHT);
        
        if (rowIndex > TEXT_ROWS) {
            rowIndex -= TEXT_ROWS + 1;
        }

        pix += 2;
        for (uint8_t columnIndex = 0; columnIndex < TEXT_COLUMNS; ++columnIndex)
        {
            // In this loop all glyph parts for specific scanline are rendered.
            st_conioCharacter *ch = conio_getCharacterBuffer(rowIndex, columnIndex);
            uint16_t foregroundColour = ch->foregroundColour;
            uint16_t backgroundColour = ch->backgroundColour;
            bool invert = ch->invert;
            uint8_t fontBits = fontdata_sun8x16[ch->locationCharacter][rowScanline];

            if (invert == true) {
                fontBits = ~fontBits;
            }

            pix[0] = (fontBits & 0x80) ? foregroundColour : backgroundColour;
            pix[1] = (fontBits & 0x40) ? foregroundColour : backgroundColour;
            pix[2] = (fontBits & 0x20) ? foregroundColour : backgroundColour;
            pix[3] = (fontBits & 0x10) ? foregroundColour : backgroundColour;
            pix[4] = (fontBits & 0x08) ? foregroundColour : backgroundColour;
            pix[5] = (fontBits & 0x04) ? foregroundColour : backgroundColour;
            pix[6] = (fontBits & 0x02) ? foregroundColour : backgroundColour;
            pix[7] = (fontBits & 0x01) ? foregroundColour : backgroundColour;
            pix += GLYPH_WIDTH;
        }

        ++pix;
        *pix = 0;
        ++pix;
        *pix = COMPOSABLE_EOL_ALIGN;
        
        pix = (uint16_t *)buffer->data;
        pix[0] = COMPOSABLE_RAW_RUN;
        pix[1] = pix[2];
        pix[2] = TEXT_COLUMNS * GLYPH_WIDTH - 2;
        buffer->data_used = (TEXT_COLUMNS * GLYPH_WIDTH + 4) / 2;
        assert(buffer->data_used < buffer->data_max);

        scanvideo_end_scanline_generation(buffer);
    } // end while(true) loop
}

void system_initialiseScanVideo(void) {

    scanvideo_setup(&vga_mode_480x272_60);
    scanvideo_timing_enable(true);
    sem_release(&videoInitialised);
    renderLoop();
}

void system_onPwmWrap(void) {
    // Clear the interrupt flag that brought us here
    pwm_clear_irq(pwm_gpio_to_slice_num(LCD_BACKLIGHT_PWM_PIN));
  
    uint16_t targetPwmValue = (systemConfiguration.lcdBacklightValue * UINT16_MAX) / 100;

    pwm_set_gpio_level(LCD_BACKLIGHT_PWM_PIN, targetPwmValue);
}

/**
    Get a pointer to the system configuration.

    @param[out]    st_systemConfiguration pointer to system configuration structure.
*/
st_systemConfiguration *system_getSystemConfiguration(void) {

    return (&systemConfiguration);
}

int main(void) {

    uint8_t msgBuffer[TEXT_COLUMNS_VISIBLE];

    set_sys_clock_khz(125000, true);

    // Setup GPIO for LED
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_LED_PIN, false);

    // Setup GPIO for LCD backlighting
    gpio_set_function(LCD_BACKLIGHT_PWM_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(LCD_BACKLIGHT_PWM_PIN);

    // Mask our slice's IRQ output into the PWM block's single interrupt line,
    // and register our interrupt handler
    pwm_clear_irq(slice_num);
    pwm_set_irq_enabled(slice_num, true);
    irq_set_exclusive_handler(PWM_IRQ_WRAP, system_onPwmWrap);
    irq_set_enabled(PWM_IRQ_WRAP, true);

    // Get some sensible defaults for the slice configuration. By default, the
    // counter is allowed to wrap over its maximum range (0 to 2**16-1)
    pwm_config config = pwm_get_default_config();
    // Set divider, reduces counter clock to sysclock/this value
    pwm_config_set_clkdiv(&config, 10.f);
    // Load the configuration into our PWM slice, and set it running.
    pwm_init(slice_num, &config, true);

    conio_initialiseCharacterBuffer(PALETTE_COLOUR_AMBER_INDEX, PALETTE_COLOUR_BLACK_INDEX);
    status_initialiseStatusBar(PALETTE_COLOUR_AMBER_INDEX, PALETTE_COLOUR_BLACK_INDEX, true);
    serial_initialiseTerminalUart(uart1);
    keyboard_initialiseKeyboard();

    // For the moment, force the LCD backlighting to 75%.
    systemConfiguration.lcdBacklightValue = 75;

    // create a semaphore to be posted when video init is complete
    sem_init(&videoInitialised, 0, 1);
    multicore_launch_core1(system_initialiseScanVideo);
    // wait for initialization of video to be complete
    sem_acquire_blocking(&videoInitialised);

	//                 012345678901234567890123456789012345678901234567890123456789
	//                 |        |         |         |         |         |         |
    conio_printString("--------------------------------[RP2040Term TFT - By PR77]--\r\n", PALETTE_COLOUR_YELLOW_INDEX, PALETTE_COLOUR_BLACK_INDEX);
    conio_printString("__________ __________      _________________________________\r\n", PALETTE_COLOUR_AMBER_INDEX, PALETTE_COLOUR_BLACK_INDEX);
    conio_printString("\\______   \\\\______   \\     \\__    ___/\\_   _____/\\__    ___/\r\n", PALETTE_COLOUR_AMBER_INDEX, PALETTE_COLOUR_BLACK_INDEX);
    conio_printString(" |       _/ |     ___/       |    |    |    __)    |    |\r\n", PALETTE_COLOUR_AMBER_INDEX, PALETTE_COLOUR_BLACK_INDEX);   
    conio_printString(" |    |   \\ |    |           |    |    |     \\     |    |\r\n", PALETTE_COLOUR_AMBER_INDEX, PALETTE_COLOUR_BLACK_INDEX);   
    conio_printString(" |____|_  / |____|     /\\    |____|    \\___  /     |____|\r\n", PALETTE_COLOUR_AMBER_INDEX, PALETTE_COLOUR_BLACK_INDEX);   
    conio_printString("        \\/             \\/                  \\/\r\n", PALETTE_COLOUR_AMBER_INDEX, PALETTE_COLOUR_BLACK_INDEX);               

	//                 012345678901234567890123456789012345678901234567890123456789
	//                 |        |         |         |         |         |         |
    conio_printString("--[GPL 3 Clause]--------------------------------[(c) 2024]--\r\n", PALETTE_COLOUR_YELLOW_INDEX, PALETTE_COLOUR_BLACK_INDEX);
    conio_printString("\r\n", PALETTE_COLOUR_WHITE_INDEX, PALETTE_COLOUR_BLACK_INDEX);

    //                 012345678901234567890123456789012345678901234567890123456789
	//                 |        |         |         |         |         |         |
    snprintf(msgBuffer, TEXT_COLUMNS_VISIBLE, "Version %s, Build %d, Release %s\r\n", CMAKE_PROJECT_VERSION, BUILD_NUMBER, CMAKE_PROJECT_DESCRIPTION);
    conio_printString(msgBuffer, PALETTE_COLOUR_YELLOW_INDEX, PALETTE_COLOUR_BLACK_INDEX);
    conio_enableCursor();

    while (true) {
        status_updateLedTask();
        conio_updateCursorTask();
        status_updateStatusBarTask();
        keyboard_updateKeyboardTask();
    }
}
