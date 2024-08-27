#include "pico.h"
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/scanvideo.h"
#include "pico/scanvideo/composable_scanline.h"
#include "conio.h"
#include "popup.h"
#include "font.h"
#include "font_sun8x16.h"
#include "font_sans8x16.h"
#include "font_serif8x16.h"
#include "font_deco8x16.h"

const st_fontEntry * availableFonts[] = {&sansFont, &serifFont, &sunFont, &decoFont};
static uint8_t fontIndex = 0;

/**
    Get the index into the active display font.

    @returns[out]  uint8_t current index into availableFonts structure.
*/
uint8_t font_getFontIndex(void) {
    
    return (fontIndex);
}

/**
    Cycle through available display fonts
*/
void font_cycleDisplayFont(void) {

    if ((fontIndex + 1) >= (sizeof(availableFonts) / sizeof(availableFonts[0]))) {
        // Ensure we never set fontIndex to out of bounds otherwise the rendering running
        // on Core 1 will index to out-of-bounds memory.
        fontIndex = 0;
    } else {
        fontIndex++;
    }

    conio_displayPopup((char *)availableFonts[fontIndex]->fontName, PALETTE_COLOUR_WHITE_INDEX, PALETTE_COLOUR_RED_INDEX, 0);
}