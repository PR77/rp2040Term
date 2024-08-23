#include "pico.h"
#include "pico/stdlib.h"
#include "string.h"
#include "time.h"
#include "pico/scanvideo.h"
#include "pico/scanvideo/composable_scanline.h"
#include "conio.h"
#include "cursor.h"
#include "popup.h"

static st_conioPopup conioPopups[MAXIMUM_POPUPS_ALLOWED];

/**
    Initialise console character buffer specific for popups
*/
void conio_initialisePopupBuffer(void) {

    memset(&conioPopups, 0, (sizeof(conioPopups) / sizeof(conioPopups[0])));
}

/**
    Displays a popup on the screen for a specified amount of time. Popups will ALWAYS be shown in the
    middle of the screen and only be 1 row high (currently).

    @param[in]     string_p string to be printed to screen.
    @param[in]     foregroundColourIndex index into palette for characters foreground colour.
    @param[in]     backgroundColourIndex index into palette for characters background colour.
    @param[in]     popupDuration duration that the popup shall be displayed for.
    @returns[out]  uint8_t UINT_8 MAX if no popup could be allocated, otherwise index into popup structure.
*/
uint8_t conio_displayPopup(char *string_p, e_colourPaletteIndexes foregroundColourIndex, e_colourPaletteIndexes backgroundColourIndex, uint16_t popupDuration) {

    uint8_t popupIndex = UINT8_MAX;
    
    // Need to find the first free entry into our popup structure.
    for (uint8_t i = 0; i < MAXIMUM_POPUPS_ALLOWED; i++) {
        if (false == conioPopups[i].popupAllocated) {
            popupIndex = i;
            break;    
        } 

        if (i == (MAXIMUM_POPUPS_ALLOWED - 1)) {
            // No free entry was found, so simply return with error code.
            return (popupIndex);
        }
    }

    // Free enty was found, it now needs to be populated with required popup content. But first,
    // fill the popup row with emtry 'SPACE' characters.
    assert (string_p != NULL);
    memset(conioPopups[popupIndex].popupRowContent, ' ', (sizeof(uint8_t) * TEXT_COLUMNS_VISIBLE));

    // Position the string to the center of the popup content.
    uint8_t popupTextLength = strnlen(string_p, TEXT_COLUMNS_VISIBLE);
    uint8_t popupTextPostionOffset = (TEXT_COLUMNS_VISIBLE - popupTextLength) >> 1;
    memcpy(&conioPopups[popupIndex].popupRowContent[popupTextPostionOffset], string_p, popupTextLength);

    // Capture the duration of the popup.
    if (popupDuration == 0) {
        conioPopups[popupIndex].remainingDisplayTime = DEFAULT_POPUP_DURATION_MS;
    } else {
        conioPopups[popupIndex].remainingDisplayTime = popupDuration;
    }

    // Copy content of existing row into popup structure buffer. 
    st_conioCharacter *source = conio_getCharacterBuffer(POPUP_ROW, 0);
    st_conioCharacter *destination = conioPopups[popupIndex].previousRowContent;

    assert (source != NULL);
    assert (destination != NULL);
    memmove(destination, source, (sizeof(st_conioCharacter) * TEXT_COLUMNS_VISIBLE));

    // Copy content of popup row into conio structure buffer with attributes. Write
    // directly to character buffer so as to not affect the cursor.
    for (uint8_t i = 0; i < TEXT_COLUMNS_VISIBLE; i++) {
        st_conioCharacter *ch = conio_getCharacterBuffer(POPUP_ROW, i);
        assert (ch != NULL);

        ch->backgroundColour = conio_getPaletteColour(backgroundColourIndex);
        ch->foregroundColour = conio_getPaletteColour(foregroundColourIndex);
        ch->invert = false;
        ch->locationCharacter = conioPopups[popupIndex].popupRowContent[i];
    }

    conioPopups[popupIndex].popupAllocated = true;
    return (popupIndex);
}

/**
    Cyclic function to handle popup task.
*/
void conio_updatePopupTask(void) {

    static uint64_t previousTime = 0;

    // Blink every interval ms
    if ((to_ms_since_boot(get_absolute_time()) - previousTime) > POPUP_UPDATE_INTERVAL_MS) {
        previousTime += POPUP_UPDATE_INTERVAL_MS;

        // Need to find active enteries in the popup structure.
        for (uint8_t i = 0; i < MAXIMUM_POPUPS_ALLOWED; i++) {
            if (true == conioPopups[i].popupAllocated) {
                if (0 == conioPopups[i].remainingDisplayTime) {
                    // Copy content of popup previous content buffer back to the conio.
                    st_conioCharacter *source = conioPopups[i].previousRowContent;
                    st_conioCharacter *destination = conio_getCharacterBuffer(POPUP_ROW, 0);

                    assert (source != NULL);
                    assert (destination != NULL);
                    memmove(destination, source, (sizeof(st_conioCharacter) * TEXT_COLUMNS_VISIBLE));
                    conioPopups[i].popupAllocated = false;
                } else {
                    if (conioPopups[i].remainingDisplayTime > POPUP_UPDATE_INTERVAL_MS) {
                        conioPopups[i].remainingDisplayTime -= POPUP_UPDATE_INTERVAL_MS;  
                    } else {
                        conioPopups[i].remainingDisplayTime = 0;
                    }
                }
            }
        }
    }
}