#include "pico.h"
#include "pico/stdlib.h"
#include "string.h"
#include "time.h"
#include "pico/scanvideo.h"
#include "pico/scanvideo/composable_scanline.h"
#include "conio.h"
#include "status.h"

/**
    Initialise status bar and support data structures.

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

    // TODO

}

/**
    Cyclic function to handle status bar specifics.
*/
void status_updateStatusBar(void) {

    static uint64_t previousTime = 0;

    // Blink every interval ms
    if ((to_ms_since_boot(get_absolute_time()) - previousTime) > STATUS_BAR_UPDATE_INTERVAL_MS) {
        previousTime += STATUS_BAR_UPDATE_INTERVAL_MS;

        status_forceStatusBarUpdate();
    }
}