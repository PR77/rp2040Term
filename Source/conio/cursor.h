#ifndef CONIO_CURSOR_H
#define	CONIO_CURSOR_H

#include "pico.h"
#include "pico/stdlib.h"
#include "pico/scanvideo.h"
#include "pico/scanvideo/composable_scanline.h"

#define DEFAULT_CURSOR_POSITION_STEPS   1
#define DEFAULT_CURSOR_TAB_STEPS        4
#define CURSOR_FLASH_INTERVAL_MS        525

typedef struct {
    uint8_t             currentCursorRow;
    uint8_t             currentCursorColumn;
    bool                cursorIsVisible;
    bool                cursorBlinkState;
} st_conioCursor;

void conio_initialiseCursorBuffer(void);
st_conioCursor *conio_getCurrentCursorPosition(void);
void conio_setNewCursorPosition(uint8_t rowIndex, uint8_t columnIndex);

/* Cursor functions */
void conio_refreshCursor(void);
void conio_enableCursor(void);
void conio_disableCursor(void);
void conio_displayCursor(void);
void conio_hideCursor(void);
void conio_moveCursorHome(void);
void conio_moveCursorUp(uint8_t positionCounts);
void conio_moveCursorDown(uint8_t positionCounts);
void conio_moveCursorLeft(uint8_t positionCounts);
void conio_moveCursorRight(uint8_t positionCounts);
void conio_updateCursorTask(void);

#endif // CONIO_CURSOR_H