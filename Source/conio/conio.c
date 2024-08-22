#include "pico.h"
#include "pico/stdlib.h"
#include "string.h"
#include "time.h"
#include "pico/scanvideo.h"
#include "pico/scanvideo/composable_scanline.h"
#include "conio.h"

static st_conioCharacter conioCharacter[TEXT_ROWS][TEXT_COLUMNS];
static st_conioPopup conioPopups[MAXIMUM_POPUPS_ALLOWED];
static st_conioCursor conioCursor;
static uint8_t defaultForegroundColourIndex;
static uint8_t defaultBackgroundColourIndex;

const uint16_t conioPalette[MAX_PALETTE_COLOURS] = {
      PICO_SCANVIDEO_PIXEL_FROM_RGB8(0x00, 0x00, 0x00),     //black
      PICO_SCANVIDEO_PIXEL_FROM_RGB8(0xFF, 0x00, 0x00),     //red
      PICO_SCANVIDEO_PIXEL_FROM_RGB8(0x00, 0xFF, 0x00),     //green
      PICO_SCANVIDEO_PIXEL_FROM_RGB8(0x00, 0x00, 0xFF),     //blue
      PICO_SCANVIDEO_PIXEL_FROM_RGB8(0xAA, 0x55, 0x00),     //brown
      PICO_SCANVIDEO_PIXEL_FROM_RGB8(0xFF, 0x00, 0xFF),     //magenta
      PICO_SCANVIDEO_PIXEL_FROM_RGB8(0x00, 0xFF, 0xFF),     //cyan
      PICO_SCANVIDEO_PIXEL_FROM_RGB8(0x55, 0x55, 0x55),     //grey
      PICO_SCANVIDEO_PIXEL_FROM_RGB8(0xFF, 0xFF, 0x55),     //yellow
      PICO_SCANVIDEO_PIXEL_FROM_RGB8(0xFF, 0xB0, 0x00),     //amber
      PICO_SCANVIDEO_PIXEL_FROM_RGB8(0xFF, 0xFF, 0xFF),     //white
};

/**
    Initialise console character buffer and support data structures.

    @param[in]     foregroundColourIndex index into palette for characters foreground colour.
    @param[in]     backgroundColourIndex index into palette for characters background colour.
*/
void conio_initialiseCharacterBuffer(e_colourPaletteIndexes foregroundColourIndex, e_colourPaletteIndexes backgroundColourIndex) {

    assert (!(foregroundColourIndex > (sizeof(conioPalette) / sizeof(conioPalette[0]))));
    assert (!(backgroundColourIndex > (sizeof(conioPalette) / sizeof(conioPalette[0]))));

    conioCursor.currentCursorRow = 0;
    conioCursor.currentCursorColumn = 0;
    conioCursor.cursorIsVisible = false;
    conioCursor.cursorBlinkState = false;

    defaultForegroundColourIndex = foregroundColourIndex;
    defaultBackgroundColourIndex = backgroundColourIndex;

    for (uint8_t i = 0; i < (TEXT_ROWS); i++)
    {
        for (uint8_t j = 0; j < (TEXT_COLUMNS); j++)
        {
            conioCharacter[i][j].foregroundColour = conio_getPaletteColour(foregroundColourIndex);
            conioCharacter[i][j].backgroundColour = conio_getPaletteColour(backgroundColourIndex);
            conioCharacter[i][j].locationCharacter = ' ';
            conioCharacter[i][j].invert = false;
        }
    }
}

/**
    Initialise console character buffer specific for popups
*/
void conio_initialisePopupBuffer(void) {

    memset(&conioPopups, 0, (sizeof(conioPopups) / sizeof(conioPopups[0])));
}

/**
    Get a pointer to the character buffer structure at a specific location.

    @param[in]     rowIndex row index into character buffer structure.
    @param[in]     columnIndex column index into character buffer structure.
    @returns[out]  st_conioCharacter pointer to character buffer structure.
*/
st_conioCharacter *conio_getCharacterBuffer(uint8_t rowIndex, uint8_t columnIndex) {

    assert (!(rowIndex > TEXT_ROWS));
    assert (!(columnIndex > TEXT_COLUMNS));

    return (&conioCharacter[rowIndex][columnIndex]);
}

/**
    Get a PICO_SCANVIDEO_PIXEL_FROM_RGB8 tripple for a specific palette index.

    @param[in]     paletteIndex index into colour palette.
    @returns[out]  uint16_t PICO_SCANVIDEO_PIXEL_FROM_RGB8 tripple.
*/
uint16_t conio_getPaletteColour(e_colourPaletteIndexes paletteIndex) {

    assert (!(paletteIndex > (sizeof(conioPalette) / sizeof(conioPalette[0]))));

    return (conioPalette[paletteIndex]);
}

/**
    Get a pointer to the cursor position structure.

    @returns[out]  st_conioCursor pointer to cursor position structure.
*/
st_conioCursor *conio_getCurrentCursorPosition(void) {

    return (&conioCursor);
}

/**
    Set a new cursor position based on row and column indexs to the screen.

    @param[in]     rowIndex row index to the screen where the cursor shall be placed.
    @param[in]     columnIndex column index to the screen where the cursor shall be placed.
*/
void conio_setNewCursorPosition(uint8_t rowIndex, uint8_t columnIndex) {

    assert (!(rowIndex > TEXT_ROWS_VISIBLE));
    assert (!(columnIndex > TEXT_COLUMNS_VISIBLE));

    conioCursor.currentCursorRow = rowIndex;
    conioCursor.currentCursorColumn = columnIndex;
}

/**
    Cyclic function to handle cursor specifics. For example flashing.
*/
void conio_updateCursorTask(void) {

    static uint64_t previousTime = 0;

    // Blink every interval ms
    if ((to_ms_since_boot(get_absolute_time()) - previousTime) > CURSOR_FLASH_INTERVAL_MS) {
        previousTime += CURSOR_FLASH_INTERVAL_MS;

        conio_refreshCursor();
    }
}

/**
    Handler function for refreshing the cursor for flashing or position changes.
*/
void conio_refreshCursor(void) {

    st_conioCursor *cursorPosition = conio_getCurrentCursorPosition();
    assert (cursorPosition != NULL);

    if (cursorPosition->cursorIsVisible == false) {
        conio_disableCursor();
        return;
    }

    if (cursorPosition->cursorBlinkState == false) {
        cursorPosition->cursorBlinkState = true;
        conio_displayCursor();
    } else {
        cursorPosition->cursorBlinkState = false;
        conio_hideCursor();        
    }
}

/**
    Enable of the cursor. Cursor will be shown during refreshing.
*/
void conio_enableCursor(void) {

    st_conioCursor *cursorPosition = conio_getCurrentCursorPosition();
    assert (cursorPosition != NULL);

    cursorPosition->cursorIsVisible = true;
}

/**
    Disable of the cursor. Cursor will be hidden and not shown during refreshing.
*/
void conio_disableCursor(void) {

    st_conioCursor *cursorPosition = conio_getCurrentCursorPosition();
    assert (cursorPosition != NULL);

    cursorPosition->cursorIsVisible = false;
}

/**
    Display of the cursor. Cursor will be visible.
*/
void conio_displayCursor(void) {

    st_conioCursor *cursorPosition = conio_getCurrentCursorPosition();
    st_conioCharacter *ch = conio_getCharacterBuffer(cursorPosition->currentCursorRow, cursorPosition->currentCursorColumn);
    assert (cursorPosition != NULL);
    assert (ch != NULL);

    ch->invert = true;
}

/**
    Hide of the cursor. Cursor will be not visible.
*/
void conio_hideCursor(void) {
    
    st_conioCursor *cursorPosition = conio_getCurrentCursorPosition();
    st_conioCharacter *ch = conio_getCharacterBuffer(cursorPosition->currentCursorRow, cursorPosition->currentCursorColumn);
    assert (cursorPosition != NULL);
    assert (ch != NULL);

    ch->invert = false;
}

/**
    Move the cursor to the home position.
*/
void conio_moveCursorHome(void) {

    st_conioCursor *cursorPosition = conio_getCurrentCursorPosition();
    assert (cursorPosition != NULL);

    conio_hideCursor();
    cursorPosition->currentCursorColumn = 0;
    cursorPosition->currentCursorRow = 0;
    
    conio_displayCursor();
}

/**
    Move the cursor up the screen. This function will perform the bounds check to ensure the
    cursor remains on the screen

    @param[in]     positionCounts numbers of steps the cursor needs to move.
*/
void conio_moveCursorUp(uint8_t positionCounts) {

    st_conioCursor *cursorPosition = conio_getCurrentCursorPosition();
    assert (cursorPosition != NULL);

    conio_hideCursor();
    if ((cursorPosition->currentCursorRow - positionCounts) >= 0) {
        cursorPosition->currentCursorRow = (cursorPosition->currentCursorRow - positionCounts);    
    } else {
        cursorPosition->currentCursorRow = 0;    
    }

    conio_displayCursor();
}

/**
    Move the cursor down the screen. This function will perform the bounds check to ensure the
    cursor remains on the screen

    @param[in]     positionCounts numbers of steps the cursor needs to move.
*/
void conio_moveCursorDown(uint8_t positionCounts) {
    
    st_conioCursor *cursorPosition = conio_getCurrentCursorPosition();
    assert (cursorPosition != NULL);

    conio_hideCursor();
    if ((cursorPosition->currentCursorRow + positionCounts) < (TEXT_ROWS_VISIBLE - 1)) {
        cursorPosition->currentCursorRow = (cursorPosition->currentCursorRow + positionCounts);    
    } else {
        cursorPosition->currentCursorRow = (TEXT_ROWS_VISIBLE - 1);
    }

    conio_displayCursor();
}

/**
    Move the cursor to the left of the screen. This function will perform the bounds check to
    ensure the cursor remains on the screen

    @param[in]     positionCounts numbers of steps the cursor needs to move.
*/
void conio_moveCursorLeft(uint8_t positionCounts) {

    st_conioCursor *cursorPosition = conio_getCurrentCursorPosition();
    assert (cursorPosition != NULL);

    conio_hideCursor();
    if ((cursorPosition->currentCursorColumn - positionCounts) >= 0) {
        cursorPosition->currentCursorColumn = (cursorPosition->currentCursorColumn - positionCounts);    
    } else {
        cursorPosition->currentCursorColumn = 0;    
    }

    conio_displayCursor();
}

/**
    Move the cursor to the right of the screen. This function will perform the bounds check to
    ensure the cursor remains on the screen

    @param[in]     positionCounts numbers of steps the cursor needs to move.
*/
void conio_moveCursorRight(uint8_t positionCounts) {

    st_conioCursor *cursorPosition = conio_getCurrentCursorPosition();
    assert (cursorPosition != NULL);

    conio_hideCursor();
    if ((cursorPosition->currentCursorColumn + positionCounts) < (TEXT_COLUMNS_VISIBLE - 1)) {
        cursorPosition->currentCursorColumn = (cursorPosition->currentCursorColumn + positionCounts);    
    } else {
        cursorPosition->currentCursorColumn = (TEXT_COLUMNS_VISIBLE - 1);
    }

    conio_displayCursor();
}

/**
    Scroll entire screen and all content "UP" by 1 TEXT_ROW.
*/
void conio_scrollScreenUp(void) {

    // Move all rows up by 1 row. Start at top and itterate down.
    for (uint8_t i = 0; i < (TEXT_ROWS_VISIBLE - 1); i++) {
        st_conioCharacter *source = conio_getCharacterBuffer(i+1, 0);
        st_conioCharacter *destination = conio_getCharacterBuffer(i, 0);
        assert (source != NULL);
        assert (destination != NULL);

        memmove(destination, source, (sizeof(st_conioCharacter) * TEXT_COLUMNS_VISIBLE));
    }

    // Clear content of last "new" row. Keep colours and other attributes as-is.
    for (uint8_t i = 0; i < TEXT_COLUMNS_VISIBLE; i++) {
        st_conioCharacter *ch = conio_getCharacterBuffer((TEXT_ROWS_VISIBLE - 1), i);
        assert (ch != NULL);
    
        ch->locationCharacter = ' ';
    }
}

/**
    Scroll entire screen and all content "DOWN" by 1 TEXT_ROW.
*/
void conio_scrollScreenDown(void) {

    // Move all rows down by 1 row. Start and bottom and itterate up.
    for (uint8_t i = (TEXT_ROWS_VISIBLE - 1); i > 0; i--) {
        st_conioCharacter *source = conio_getCharacterBuffer(i-1, 0);
        st_conioCharacter *destination = conio_getCharacterBuffer(i, 0);
        assert (source != NULL);
        assert (destination != NULL);

        memmove(destination, source, (sizeof(st_conioCharacter) * TEXT_COLUMNS_VISIBLE));
    }

    // Clear content of first "new" row. Keep colours and other attributes as-is.
    for (uint8_t i = 0; i < TEXT_COLUMNS_VISIBLE; i++) {
        st_conioCharacter *ch = conio_getCharacterBuffer(0, i);
        assert (ch != NULL);
    
        ch->locationCharacter = ' ';
    }
}

/**
    Clear the entire screen and move the cursor back to home position.
*/
void conio_clearScreenHomeCursor(void) {
    
    conioCursor.currentCursorRow = 0;
    conioCursor.currentCursorColumn = 0;

    for (uint8_t i = 0; i < (TEXT_ROWS_VISIBLE); i++)
    {
        for (uint8_t j = 0; j < (TEXT_COLUMNS); j++)
        {
            conioCharacter[i][j].foregroundColour = conio_getPaletteColour(defaultForegroundColourIndex);
            conioCharacter[i][j].backgroundColour = conio_getPaletteColour(defaultBackgroundColourIndex);
            conioCharacter[i][j].locationCharacter = ' ';
            conioCharacter[i][j].invert = false;
        }
    }
}

/**
    Print a character to the character buffer structure with specific attributes.

    @param[in]     character character to be printed to screen.
    @param[in]     foregroundColourIndex index into palette for characters foreground colour.
    @param[in]     backgroundColourIndex index into palette for characters background colour.
*/
void conio_printCharacter(char character, e_colourPaletteIndexes foregroundColourIndex, e_colourPaletteIndexes backgroundColourIndex) {

    st_conioCursor *cursorPosition = conio_getCurrentCursorPosition();
    assert (cursorPosition != NULL);

    conio_hideCursor();

    if ((character >= 0x00) && (character <= 0x1F)) {
        // If it is not a printable character, then we need to check for control characters
        if (character == '\r') {
            cursorPosition->currentCursorColumn = 0;
        }

        if ((character == '\n') && (cursorPosition->currentCursorRow <= (TEXT_ROWS_VISIBLE - 1))) {
            cursorPosition->currentCursorRow++;

            if (cursorPosition->currentCursorRow >= TEXT_ROWS_VISIBLE) {
                conio_scrollScreenUp();
                cursorPosition->currentCursorRow--;
            }
        } 

        if ((character == '\t') && (cursorPosition->currentCursorColumn <= (TEXT_COLUMNS_VISIBLE - 4))) {
            cursorPosition->currentCursorColumn += 4;
        }

        if ((character == '\b') && (cursorPosition->currentCursorColumn > 0)) {
            cursorPosition->currentCursorColumn--;
        }
    } else {
        if ((character >= ' ') && (character <= 0x7F)) {
            // Check if the character is printable, if so now finally print it. However,
            // firstly check if we are at the screen bounds. If so, then screen scroll up the previous lines.

            st_conioCharacter *ch = conio_getCharacterBuffer(cursorPosition->currentCursorRow, cursorPosition->currentCursorColumn);
            assert (ch != NULL);
            
            ch->foregroundColour = conio_getPaletteColour(foregroundColourIndex);
            ch->backgroundColour = conio_getPaletteColour(backgroundColourIndex);
            ch->locationCharacter = character;

            cursorPosition->currentCursorColumn++;

            if (cursorPosition->currentCursorColumn >= TEXT_COLUMNS_VISIBLE) {
                cursorPosition->currentCursorColumn = 0;

                // Check that the row will not increment to the status bar and potentially overwrite the
                // first character of the status bar.
                if (cursorPosition->currentCursorRow < TEXT_ROWS_VISIBLE) {
                    cursorPosition->currentCursorRow++;
                }  
            }

            if (cursorPosition->currentCursorRow >= TEXT_ROWS_VISIBLE) {
                conio_scrollScreenUp();
                cursorPosition->currentCursorRow--;
            }
        }
    }
}

/**
    Print a string to the character buffer structure with specific attributes.

    @param[in]     string_p string to be printed to screen.
    @param[in]     foregroundColourIndex index into palette for characters foreground colour.
    @param[in]     backgroundColourIndex index into palette for characters background colour.
*/
void conio_printString(char *string_p, e_colourPaletteIndexes foregroundColourIndex, e_colourPaletteIndexes backgroundColourIndex) {

    assert (string_p != NULL);

    while(*string_p) {
        conio_printCharacter(*string_p++, foregroundColourIndex, backgroundColourIndex);  
    }
}

/**
    Print a character to the character buffer structure without any specific attributes.
    Defaults for the foreground and background colours are used.

    @param[in]     character character to be printed to screen.
*/
void conio_printSimpleCharacter(char character) {
    
    conio_printCharacter(character, defaultForegroundColourIndex, defaultBackgroundColourIndex);
}

/**
    Print a string to the character buffer structure without any specific attributes.
    Defaults for the foreground and background colours are used.

    @param[in]     string_p string to be printed to screen.
*/
void conio_printSimpleString(char *string_p) {
    
    assert (string_p != NULL);

    while(*string_p) {
        conio_printSimpleCharacter(*string_p++);  
    }
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
    st_conioCharacter *source = NULL;
    st_conioCharacter *destination = NULL;
    
    // Need to find the first free entry into our popup structure.
    for (uint8_t i = 0; i < MAXIMUM_POPUPS_ALLOWED; i++) {
        if (false == conioPopups[i].popupAllocated) {
            popupIndex = i;
            break;    
        } else {

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
    source = conio_getCharacterBuffer(POPUP_ROW, 0);
    destination = conioPopups[popupIndex].previousRowContent;

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