#include "pico.h"
#include "pico/stdlib.h"
#include "string.h"
#include "time.h"
#include "pico/scanvideo.h"
#include "pico/scanvideo/composable_scanline.h"
#include "conio.h"

static st_conioCharacter conioCharacter[TEXT_ROWS][TEXT_COLUMNS];
static st_conioCursor conioCursor;

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
    Print a character to the character buffer structure with specific attributes.

    @param[in]     character character to be printed to screen.
    @param[in]     foregroundColourIndex index into palette for characters foreground colour.
    @param[in]     backgroundColourIndex index into palette for characters background colour.
*/
void conio_printCharacter(uint8_t character, e_colourPaletteIndexes foregroundColourIndex, e_colourPaletteIndexes backgroundColourIndex) {

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
void conio_printString(uint8_t *string_p, e_colourPaletteIndexes foregroundColourIndex, e_colourPaletteIndexes backgroundColourIndex) {

    assert (string_p != NULL);

    while(*string_p) {
        conio_printCharacter(*string_p++, foregroundColourIndex, backgroundColourIndex);  
    }
}