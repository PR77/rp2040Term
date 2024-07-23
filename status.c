#include "pico.h"
#include "pico/stdlib.h"
#include "string.h"
#include "time.h"
#include "pico/scanvideo.h"
#include "pico/scanvideo/composable_scanline.h"
#include "main.h"
#include "system.h"
#include "conio.h"
#include "serial.h"
#include "keyboard.h"
#include "status.h"

/**
    Initialise status bar and support data structures. The status bar has a fixed structure
    and content.

    @param[in]     foregroundColourIndex index into palette for characters foreground colour.
    @param[in]     backgroundColourIndex index into palette for characters background colour.
    @param[in]     backgroundColourIndex index into palette for characters background colour.

*/
void status_initialiseStatusBar(e_colourPaletteIndexes foregroundColourIndex, e_colourPaletteIndexes backgroundColourIndex, bool invertText) {

    assert (!(foregroundColourIndex > (sizeof(conioPalette) / sizeof(conioPalette[0]))));
    assert (!(backgroundColourIndex > (sizeof(conioPalette) / sizeof(conioPalette[0]))));

    for (uint8_t i = 0; i < (TEXT_COLUMNS_VISIBLE); i++)
    {
        st_conioCharacter *ch = conio_getCharacterBuffer(STATUS_BAR_ROW, i);
        assert (ch != NULL);

        ch->foregroundColour = conio_getPaletteColour(foregroundColourIndex);
        ch->backgroundColour = conio_getPaletteColour(backgroundColourIndex);
        ch->locationCharacter = ' ';
        ch->invert = invertText;
    }
}

/**
    Force an update of the status bar.
*/
void status_forceStatusBarUpdate(void) {

    uint8_t msgBuffer[TEXT_COLUMNS_VISIBLE + 1];
    // Need to use TEXT_COLUMNS_VISIBLE + 1 to allow for the snprintf to add a NULL terminator.

    st_serialConfiguration *serial = serial_getSerialConfiguration();
    st_systemConfiguration *system = system_getSystemConfiguration();
    st_keyboardConfiguration *keyboard = keyboard_getKeyboardConfiguration();
    assert (serial != NULL);
    assert (system != NULL);
    assert (keyboard != NULL);

    memset(msgBuffer, 0, (sizeof(msgBuffer) / sizeof(msgBuffer[0])));

	// 012345678901234567890123456789012345678901234567890123456789
	// |        |         |         |         |         |         |
    // AAAAAA BCD, Map EE, BL FFF%, Beeper GGG, HHHH, USB: IIIIIIII
    //
    //     AAAAAA = Baudrate (6 characters fixed length)                                                 
    //          B = Databits (1 character fixed length)
    //          C = Databits (1 character fixed length)
    //          D = Databits (1 character fixed length)
    //         EE = Keymap (2 characters fixed length)
    //        FFF = LCD Backlight percentage (3 characters fixed length)
    //        GGG = Beeper / sounds on or off (3 characters fixed length)
    //       HHHH = Local Echo (4 characters fixed length)
    //   IIIIIIII = USB details (8 characters fixed length)

    uint8_t serialParity = '?';
	switch (serial->parity) {
        case UART_PARITY_NONE:
            serialParity = 'N';
            break;
        case UART_PARITY_ODD:
            serialParity = 'O';
            break;
        case UART_PARITY_EVEN:
            serialParity = 'E';
            break;
	}

    snprintf(msgBuffer, TEXT_COLUMNS_VISIBLE, "%06d %i%c%i, ", serial->baudRate, serial->dataBits, serialParity, serial->stopBits);
    snprintf(&msgBuffer[12], (TEXT_COLUMNS_VISIBLE - 12), "Map ");
    snprintf(&msgBuffer[16], (TEXT_COLUMNS_VISIBLE - 16), "%s, ", KEYMAP);
    snprintf(&msgBuffer[20], (TEXT_COLUMNS_VISIBLE - 20), "BL %03d%%, ", system->lcdBacklightValue);
    snprintf(&msgBuffer[29], (TEXT_COLUMNS_VISIBLE - 29), "Beeper %s, ", (system->beeper == true) ? " On" : "Off");
    snprintf(&msgBuffer[41], (TEXT_COLUMNS_VISIBLE - 41), "%s, ", (system->localEcho == true) ? "Echo" : "    ");
    snprintf(&msgBuffer[47], (TEXT_COLUMNS_VISIBLE - 47 + 1), "USB: %s", keyboard->deviceStr);

    // Print status bar content directly to character buffer so as to not affect the cursor.
    for (uint8_t i = 0; i < TEXT_COLUMNS_VISIBLE; i++) {
        st_conioCharacter *ch = conio_getCharacterBuffer(STATUS_BAR_ROW, i);
        assert (ch != NULL);
    
        if (msgBuffer[i] != 0) {
            ch->locationCharacter = msgBuffer[i];
        } else {
            ch->locationCharacter = ' ';
        }
    }
}

/**
    Cyclic function to handle status bar specifics.
*/
void status_updateStatusBarTask(void) {

    static uint64_t previousTime = 0;

    // Blink every interval ms
    if ((to_ms_since_boot(get_absolute_time()) - previousTime) > STATUS_BAR_UPDATE_INTERVAL_MS) {
        previousTime += STATUS_BAR_UPDATE_INTERVAL_MS;

        status_forceStatusBarUpdate();
    }
}

/**
    Cyclic function to handle status / heartbeat LED flashing.
*/
void status_updateLedTask(void) {
    static uint64_t previousTime = 0;
    static bool currentLedState = false;

    // Blink every interval ms
    if ((to_ms_since_boot(get_absolute_time()) - previousTime) > LED_FLASH_INTERVAL_MS) {
        previousTime += LED_FLASH_INTERVAL_MS;
        
        gpio_put(PICO_DEFAULT_LED_PIN, currentLedState);
        currentLedState ^= true;
    }
}