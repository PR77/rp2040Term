#ifndef CONIO_H
#define	CONIO_H

#include "pico.h"
#include "pico/stdlib.h"
#include "pico/scanvideo.h"
#include "pico/scanvideo/composable_scanline.h"

#define CONIO_EXTENDED_ASCII_CHARACTER_SET

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

typedef struct {
    uint16_t            foregroundColour;
    uint16_t            backgroundColour;
    char                locationCharacter;
    bool                invert;
} st_conioCharacter;

extern const uint16_t conioPalette[MAX_PALETTE_COLOURS];

void conio_initialiseCharacterBuffer(e_colourPaletteIndexes foregroundColourIndex, e_colourPaletteIndexes backgroundColourIndex);
void conio_initialisePopupBuffer(void);
st_conioCharacter *conio_getCharacterBuffer(uint8_t rowIndex, uint8_t columnIndex);
uint16_t conio_getPaletteColour(uint8_t paletteIndex);
void conio_clearScreenHomeCursor(void);

/* Scroll functions */
void conio_scrollScreenUp(void);
void conio_scrollScreenDown(void);

/* Print functions */
void conio_printCharacter(char character, e_colourPaletteIndexes foregroundColourIndex, e_colourPaletteIndexes backgroundColourIndex);
void conio_printString(char *string_p, e_colourPaletteIndexes foregroundColourIndex, e_colourPaletteIndexes backgroundColourIndex);
void conio_printSimpleCharacter(char character);
void conio_printSimpleString(char *string_p);

#endif // CONIO_H