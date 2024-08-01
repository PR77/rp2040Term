#ifndef CONIO_H
#define	CONIO_H

#include "pico.h"
#include "pico/stdlib.h"
#include "pico/scanvideo.h"
#include "pico/scanvideo/composable_scanline.h"

typedef enum {
    PALETTE_COLOUR_BLACK_INDEX = 0,
    PALETTE_COLOUR_RED_INDEX,
    PALETTE_COLOUR_GREEN_INDEX,
    PALETTE_COLOUR_BLUE_INDEX,
    PALETTE_COLOUR_BROWN_INDEX,
    PALETTE_COLOUR_MAGENTA_INDEX,
    PALETTE_COLOUR_CYAN_INDEX,
    PALETTE_COLOUR_GREY_INDEX,
    PALETTE_COLOUR_YELLOW_INDEX,
    PALETTE_COLOUR_AMBER_INDEX,
    PALETTE_COLOUR_WHITE_INDEX,
    MAX_PALETTE_COLOURS
} e_colourPaletteIndexes;

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

extern const uint16_t conioPalette[MAX_PALETTE_COLOURS];

void conio_initialiseCharacterBuffer(e_colourPaletteIndexes foregroundColourIndex, e_colourPaletteIndexes backgroundColourIndex);
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
void conio_printCharacter(uint8_t character, e_colourPaletteIndexes foregroundColourIndex, e_colourPaletteIndexes backgroundColourIndex);
void conio_printSimpleCharacter(uint8_t character);
void conio_printString(uint8_t *string_p, e_colourPaletteIndexes foregroundColourIndex, e_colourPaletteIndexes backgroundColourIndex);

#endif // CONIO_H