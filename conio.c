#include "pico.h"
#include "pico/stdlib.h"
#include "string.h"
#include "time.h"
#include "pico/scanvideo.h"
#include "pico/scanvideo/composable_scanline.h"
#include "conio.h"

static st_conioCharacter conioCharacter[TEXT_ROWS][TEXT_COLUMNS];
static st_conioCursor conioCursor;

static const uint16_t conioPalette[] = {
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

static uint8_t topRow;

void conio_initialiseCharacterBuffer(uint8_t foregroundColourIndex, uint8_t backgroundColourIndex) {

    assert (!(foregroundColourIndex > (sizeof(conioPalette) / sizeof(conioPalette[0]))));
    assert (!(backgroundColourIndex > (sizeof(conioPalette) / sizeof(conioPalette[0]))));

    topRow = 0;

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

st_conioCharacter *conio_getCharacterBuffer(uint8_t rowIndex, uint8_t columnIndex) {

    assert (!(rowIndex > TEXT_ROWS));
    assert (!(columnIndex > TEXT_COLUMNS));

    return (&conioCharacter[rowIndex][columnIndex]);
}

uint16_t conio_getPaletteColour(uint8_t paletteIndex) {

    assert (!(paletteIndex > (sizeof(conioPalette) / sizeof(conioPalette[0]))));

    return (conioPalette[paletteIndex]);
}

st_conioCursor *conio_getCurrentCursorPosition(void) {

    return (&conioCursor);
}

void conio_setNewCursorPosition(uint8_t rowIndex, uint8_t columnIndex) {

    assert (!(rowIndex > TEXT_ROWS));
    assert (!(columnIndex > TEXT_COLUMNS));

    conioCursor.currentCursorRow = rowIndex;
    conioCursor.currentCursorColumn = columnIndex;
}

// ----------------------------------------------------------------------------

void conio_updateCursorTask(void) {

    static uint64_t previousTime = 0;

    // Blink every interval ms
    if ((to_ms_since_boot(get_absolute_time()) - previousTime) > CURSOR_FLASH_INTERVAL_MS) {
        previousTime += CURSOR_FLASH_INTERVAL_MS;

        conio_refreshCursor();
    }
}

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

void conio_enableCursor(void) {

    st_conioCursor *cursorPosition = conio_getCurrentCursorPosition();
    assert (cursorPosition != NULL);

    cursorPosition->cursorIsVisible = true;
}

void conio_disableCursor(void) {

    st_conioCursor *cursorPosition = conio_getCurrentCursorPosition();
    assert (cursorPosition != NULL);

    cursorPosition->cursorIsVisible = false;
}

void conio_displayCursor(void) {

    st_conioCursor *cursorPosition = conio_getCurrentCursorPosition();
    st_conioCharacter *ch = conio_getCharacterBuffer(cursorPosition->currentCursorRow, cursorPosition->currentCursorColumn);
    assert (cursorPosition != NULL);
    assert (ch != NULL);

    ch->invert = true;
}

void conio_hideCursor(void) {
    
    st_conioCursor *cursorPosition = conio_getCurrentCursorPosition();
    st_conioCharacter *ch = conio_getCharacterBuffer(cursorPosition->currentCursorRow, cursorPosition->currentCursorColumn);
    assert (cursorPosition != NULL);
    assert (ch != NULL);

    ch->invert = false;
}

void conio_scrollScreenUp(void) {

    for (uint8_t i = 0; i < (TEXT_ROWS - 1); i++) {
        st_conioCharacter *destination = conio_getCharacterBuffer(i, 0);
        st_conioCharacter *source = conio_getCharacterBuffer(i+1, 0);
        assert (destination != NULL);
        assert (source != NULL);

        memmove(destination, source, (sizeof(st_conioCharacter) * TEXT_COLUMNS));
    }

    for (uint8_t i = 0; i < TEXT_COLUMNS; i++) {
        st_conioCharacter *ch = conio_getCharacterBuffer((TEXT_ROWS - 1), i);
        assert (ch != NULL);
    
        ch->locationCharacter = ' ';
    }
}

void conio_scrollScreenDown(void) {

}

void conio_printCharacter(uint8_t character, uint8_t foregroundColourIndex, uint8_t backgroundColourIndex) {

    st_conioCursor *cursorPosition = conio_getCurrentCursorPosition();
    assert (cursorPosition != NULL);

    conio_hideCursor();

    if ((character >= 0x00) && (character <= 0x1F)) {
        // If it is not a printable character, then we need to check for control characters
        if (character == '\r') {
            cursorPosition->currentCursorColumn = 0;
        }

        if ((character == '\n') && (cursorPosition->currentCursorRow <= (TEXT_ROWS - 1))) {
            cursorPosition->currentCursorRow++;
        }

        if ((character == '\t') && (cursorPosition->currentCursorColumn <= (TEXT_COLUMNS - 4))) {
            cursorPosition->currentCursorColumn += 4;
        }

        if ((character == '\b') && (cursorPosition->currentCursorColumn > 0)) {
            cursorPosition->currentCursorColumn--;
        }
    } else {
        if ((character >= ' ') && (character <= 0x7F)) {
            // Check if the character is printable, if so now finally print it. However,
            // firstly check if we are at the screen bounds. If so, then screen scroll up the previous lines.

            if (cursorPosition->currentCursorColumn >= TEXT_COLUMNS) {
                cursorPosition->currentCursorColumn = 0;
                cursorPosition->currentCursorRow++;  
            }

            if (cursorPosition->currentCursorRow >= TEXT_ROWS) {
                conio_scrollScreenUp();
                cursorPosition->currentCursorRow--;
            }

            st_conioCharacter *ch = conio_getCharacterBuffer(cursorPosition->currentCursorRow, cursorPosition->currentCursorColumn);
            assert (ch != NULL);
            
            ch->foregroundColour = conio_getPaletteColour(foregroundColourIndex);
            ch->backgroundColour = conio_getPaletteColour(backgroundColourIndex);
            ch->locationCharacter = character;

            cursorPosition->currentCursorColumn++;
        }
    }
}

void conio_printString(uint8_t *string_p, uint8_t foregroundColourIndex, uint8_t backgroundColourIndex) {

    assert (string_p != NULL);

    while(*string_p) {
        conio_printCharacter(*string_p++, foregroundColourIndex, backgroundColourIndex);  
    }
}