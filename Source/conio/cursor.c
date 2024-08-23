#include "pico.h"
#include "pico/stdlib.h"
#include "string.h"
#include "time.h"
#include "pico/scanvideo.h"
#include "pico/scanvideo/composable_scanline.h"
#include "conio.h"
#include "cursor.h"
#include "popup.h"

static st_conioCursor conioCursor;

/**
    Initialise console character buffer specific for cursor.
*/
void conio_initialiseCursorBuffer(void) {

    memset(&conioCursor, 0, (sizeof(st_conioCursor)));
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

    st_conioCursor *cursorPosition = conio_getCurrentCursorPosition();
    assert (cursorPosition != NULL);
    assert (!(rowIndex > TEXT_ROWS_VISIBLE));
    assert (!(columnIndex > TEXT_COLUMNS_VISIBLE));

    cursorPosition->currentCursorRow = rowIndex;
    cursorPosition->currentCursorColumn = columnIndex;
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
    assert (cursorPosition != NULL);

    st_conioCharacter *ch = conio_getCharacterBuffer(cursorPosition->currentCursorRow, cursorPosition->currentCursorColumn);
    assert (ch != NULL);

    ch->invert = true;
}

/**
    Hide of the cursor. Cursor will be not visible.
*/
void conio_hideCursor(void) {
    
    st_conioCursor *cursorPosition = conio_getCurrentCursorPosition();
    assert (cursorPosition != NULL);

    st_conioCharacter *ch = conio_getCharacterBuffer(cursorPosition->currentCursorRow, cursorPosition->currentCursorColumn);
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
