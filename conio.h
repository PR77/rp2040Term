#ifndef CONIO_H
#define	CONIO_H

#include "pico.h"
#include "pico/stdlib.h"
#include "pico/scanvideo.h"
#include "pico/scanvideo/composable_scanline.h"

#define PALETTE_COLOUR_BLACK_INDEX      0
#define PALETTE_COLOUR_RED_INDEX        1
#define PALETTE_COLOUR_GREEN_INDEX      2
#define PALETTE_COLOUR_BLUE_INDEX       3
#define PALETTE_COLOUR_BROWN_INDEX      4
#define PALETTE_COLOUR_MAGENTA_INDEX    5
#define PALETTE_COLOUR_CYAN_INDEX       6
#define PALETTE_COLOUR_GREY_INDEX       7
#define PALETTE_COLOUR_YELLOW_INDEX     8
#define PALETTE_COLOUR_AMBER_INDEX      9
#define PALETTE_COLOUR_WHITE_INDEX      10

#define CURSOR_FLASH_INTERVAL_MS        525

typedef struct {
    uint16_t   foregroundColour;
    uint16_t   backgroundColour;
    uint8_t    locationCharacter;
    bool       invert;
} st_conioCharacter;

typedef struct {
    uint8_t     currentCursorRow;
    uint8_t     currentCursorColumn;
    bool        cursorIsVisible;
    bool        cursorBlinkState;
} st_conioCursor;

void conio_initialiseCharacterBuffer(uint8_t foregroundColourIndex, uint8_t backgroundColourIndex);
st_conioCharacter *conio_getCharacterBuffer(uint8_t rowIndex, uint8_t columnIndex);
uint16_t conio_getPaletteColour(uint8_t paletteIndex);
st_conioCursor *conio_getCurrentCursorPosition(void);
void conio_setNewCursorPosition(uint8_t rowIndex, uint8_t columnIndex);

/* Cursor functions */
void conio_updateCursorTask(void);
void conio_refreshCursor(void);
void conio_enableCursor(void);
void conio_disableCursor(void);
void conio_displayCursor(void);
void conio_hideCursor(void);

/* Scroll functions */
void conio_scrollScreenUp(void);
void conio_scrollScreenDown(void);

/* Print functions */
void conio_printCharacter(uint8_t character, uint8_t foregroundColourIndex, uint8_t backgroundColourIndex);
void conio_printString(uint8_t *string_p, uint8_t foregroundColourIndex, uint8_t backgroundColourIndex);

#endif // CONIO_H