#include "pico.h"
#include "pico/stdlib.h"
#include "pico/scanvideo.h"
#include "pico/scanvideo/composable_scanline.h"
#include "hardware/clocks.h"
#include "hardware/watchdog.h"
#include "hardware/irq.h"
#include "hardware/pwm.h"
#include "RP2040-PWM-DMA-Audio/audio.h"
#include "StartupAppleII.h"
#include "font_sun8x16.h"
#include "conio.h"
#include "serial.h"
#include "system.h"

static st_systemConfiguration systemConfiguration;

const scanvideo_timing_t tftLQ043Timing_480x272_50 = {

    .clock_freq = LCD_PIXEL_CLK,

    .h_active = 480,
    .v_active = 272,

    .h_front_porch = 2,
    .h_pulse = 96,
    .h_total = 598,
    .h_sync_polarity = 1,

    .v_front_porch = 2,
    .v_pulse = 2,
    .v_total = 305,
    .v_sync_polarity = 1,

    .enable_clock = 1,
    .clock_polarity = 1,

    .enable_den = 0
};    

const scanvideo_mode_t tftLQ042_480x272_60 = {

    .default_timing = &tftLQ043Timing_480x272_50,
    .pio_program = &video_24mhz_composable,
    .width = 480,
    .height = 272,
    .xscale = 1,
    .yscale = 1,
}; 

/**
    Initialise system and support data structures.
*/
void system_initialiseSystem(void) {

    systemConfiguration.lcdBacklightValue = LCD_BACKLIGHTING_DEFAULT;
    systemConfiguration.beeper = false;

    // Setup GPIO for LED
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_LED_PIN, false);

    // Setup GPIO for LCD PON
    gpio_init(LCD_RESET_PON_PIN);
    gpio_set_dir(LCD_RESET_PON_PIN, GPIO_OUT);
    gpio_put(LCD_RESET_PON_PIN, true);  

    // Setup GPIO for LCD Horizontal Scanning Direction
    gpio_init(LCD_HRV_VRV_SCANNING_PIN);
    gpio_set_dir(LCD_HRV_VRV_SCANNING_PIN, GPIO_OUT);
    gpio_put(LCD_HRV_VRV_SCANNING_PIN, true);  

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
}

void system_initialiseAudioPlayer(void) {

    // Beep audio sample taken from https://froods.ca/~dschaub/sound.html.
    // Check Tools folder for HTML converter based on bitluni's audio to header converter. Original
    // can be found here https://bitluni.net/wp-content/uploads/2018/01/Audio2Header.html.

    audio_init(SOUND_OUTPUT_PIN, sampleRate);
}

/**
    Get a pointer to the system configuration.

    @returns[out]  st_systemConfiguration pointer to system configuration structure.
*/
st_systemConfiguration *system_getSystemConfiguration(void) {

    return (&systemConfiguration);
}

/**
    Toggle beeper ON or OFF. Confirmation beep when beeper is ON.
*/
void system_toggleBeeper(void) {

    systemConfiguration.beeper ^= true;

    if (true == systemConfiguration.beeper) {
        audio_play_once(samples, sizeof(samples));    
    }
}

/**
    Toggle local echo ON or OFF.
*/
void system_toggleLocalEcho(void) {

    systemConfiguration.localEcho ^= true;
}

/**
    Toggle inserting line feed when carriage return is detected.
*/
void system_toggleCRLF(void) {

    systemConfiguration.insertLineFeedOnCarriageReturn ^= true;
}

/**
    Increase backlight value by 1 step size.
*/
void system_increaseBacklightByStep(void) {

    if (systemConfiguration.lcdBacklightValue < LCD_BACKLIGHTING_PWM_MAX) {
        systemConfiguration.lcdBacklightValue += LCD_BACKLIGHTING_STEP_SIZE;
    }
}

/**
    Decrease backlight value by 1 step size.
*/
void system_decreaseBacklightByStep(void) {
    
    if (systemConfiguration.lcdBacklightValue > LCD_BACKLIGHTING_STEP_SIZE) {
        systemConfiguration.lcdBacklightValue -= LCD_BACKLIGHTING_STEP_SIZE;
    }
}

/**
    PWM wrap handler. Needed to ensure PWM duty cycle is updated on edge.
*/
void system_onPwmWrap(void) {
    // Clear the interrupt flag that brought us here
    pwm_clear_irq(pwm_gpio_to_slice_num(LCD_BACKLIGHT_PWM_PIN));
  
    uint16_t targetPwmValue = (systemConfiguration.lcdBacklightValue * UINT16_MAX) / LCD_BACKLIGHTING_PWM_MAX;

    if (targetPwmValue == UINT16_MAX) {
        targetPwmValue -= 1;
    }

    pwm_set_gpio_level(LCD_BACKLIGHT_PWM_PIN, targetPwmValue);
}

/**
    System reset handler. Execute software triggered system reset.
*/
void system_executeSystemReset(void) {

    watchdog_reboot(0, 0, 0);
}

/**
    Keyboard and UART transmit routing handler. This is used to handle printing
    to conio (if local echo is enable) and sending the character data from
    keyboard entry to UART tramit buffer.
*/
void system_handleKeyboardAndUartTransmitRouting(uint8_t character) {

    serial_uartSendCharacter(character);

    if (true == systemConfiguration.localEcho) {
        conio_printSimpleCharacter(character);    
    }

    if ((true == systemConfiguration.insertLineFeedOnCarriageReturn) && (character == '\r')) {
        conio_printSimpleString("\r\n");
    }
}

/**
    Main render loop for scan video. Executed on VIDEO_RENDERING_CORE core.
*/
void __time_critical_func(system_renderLoop)(void) {
    
    uint coreNumber = get_core_num();
    // Ensure rendering is running on VIDEO_RENDERING_CORE
    assert ((coreNumber > 0) && (coreNumber < 2));

    while (true) {
        struct scanvideo_scanline_buffer *buffer = scanvideo_begin_scanline_generation(true);
        uint16_t *pix = (uint16_t *)buffer->data;
        uint16_t scanlineNumber = scanvideo_scanline_number(buffer->scanline_id);
        uint8_t rowIndex = (scanlineNumber / GLYPH_HEIGHT);
        uint8_t rowScanline = (scanlineNumber % GLYPH_HEIGHT);
        
        if (rowIndex > TEXT_ROWS) {
            rowIndex -= TEXT_ROWS + 1;
        }

        pix += 2;

        // Assign ch pointer for the applicable row and increment the column index within the
        // for-loop. This avoids having to call conio_getCharacterBuffer from within the loop.
        st_conioCharacter *ch = conio_getCharacterBuffer(rowIndex, 0);

        for (uint8_t columnIndex = 0; columnIndex < TEXT_COLUMNS; ++columnIndex) {
            // In this loop all glyph parts for specific scanline are rendered.
            uint16_t foregroundColour = ch->foregroundColour;
            uint16_t backgroundColour = ch->backgroundColour;
            uint8_t fontBits = fontdata_sun8x16[ch->locationCharacter][rowScanline];

            if (ch->invert == true) {
                fontBits = ~fontBits;
            }

            // Seems "normal" if-else statements are faster than ternary. Need to look into the
            // assembler output to understand why. 

            /*
            pix[0] = (fontBits & 0x80) ? foregroundColour : backgroundColour;
            pix[1] = (fontBits & 0x40) ? foregroundColour : backgroundColour;
            pix[2] = (fontBits & 0x20) ? foregroundColour : backgroundColour;
            pix[3] = (fontBits & 0x10) ? foregroundColour : backgroundColour;
            pix[4] = (fontBits & 0x08) ? foregroundColour : backgroundColour;
            pix[5] = (fontBits & 0x04) ? foregroundColour : backgroundColour;
            pix[6] = (fontBits & 0x02) ? foregroundColour : backgroundColour;
            pix[7] = (fontBits & 0x01) ? foregroundColour : backgroundColour;
            */

            if (fontBits & 0x80) {
                pix[0] = foregroundColour;
            } else pix[0] = backgroundColour;

            if (fontBits & 0x40) {
                pix[1] = foregroundColour;
            } else pix[1] = backgroundColour;

            if (fontBits & 0x20) {
                pix[2] = foregroundColour;
            } else pix[2] = backgroundColour;

            if (fontBits & 0x10) {
                pix[3] = foregroundColour;
            } else pix[3] = backgroundColour;

            if (fontBits & 0x08) {
                pix[4] = foregroundColour;
            } else pix[4] = backgroundColour;

            if (fontBits & 0x04) {
                pix[5] = foregroundColour;
            } else pix[5] = backgroundColour;

            if (fontBits & 0x02) {
                pix[6] = foregroundColour;
            } else pix[6] = backgroundColour;

            if (fontBits & 0x01) {
                pix[7] = foregroundColour;
            } else pix[7] = backgroundColour;

            // Incremet the pix pointer by the GLYPH_WIDTH.
            pix += GLYPH_WIDTH;
            // Increment the ch pointer to the next column.
            ch++;
        }

        ++pix;
        *pix = 0;
        ++pix;
        *pix = COMPOSABLE_EOL_ALIGN;
        
        pix = (uint16_t *)buffer->data;
        pix[0] = COMPOSABLE_RAW_RUN;

        // Populate pix[1] with actual pixel data which was in pix[2] as this location will be overwritten.
        // COMPOSABLE_RAW_RUN
        // 3 or more (N) separately colored pixels (use | RAW_RUN | COLOR1 | N-3 | COLOR2 | COLOR3 …​ | COLOR(N) |)
        // Note that the first color appears before the count (otherwise it would not be possible to achieve the timing required)
        // https://github.com/raspberrypi/pico-extras/tree/master/src/common/pico_scanvideo#readme
        pix[1] = pix[2];
        pix[2] = TEXT_COLUMNS * GLYPH_WIDTH - 2;
        buffer->data_used = (TEXT_COLUMNS * GLYPH_WIDTH + 4) / 2;
        buffer->status = SCANLINE_OK;
        assert(buffer->data_used < buffer->data_max);

        scanvideo_end_scanline_generation(buffer);
    } // end while(true) loop
}

/**
    Cyclic function to handle status / heartbeat LED flashing.
*/
void system_updateLedTask(void) {
    static uint64_t previousTime = 0;
    static bool currentLedState = false;

    // Blink every interval ms
    if ((to_ms_since_boot(get_absolute_time()) - previousTime) > LED_FLASH_INTERVAL_MS) {
        previousTime += LED_FLASH_INTERVAL_MS;
        
        gpio_put(PICO_DEFAULT_LED_PIN, currentLedState);
        currentLedState ^= true;
    }
}

/**
    Cyclic function to handle bell task.
*/
void system_updateBellTask(void) {

    audio_mixer_step();
}
