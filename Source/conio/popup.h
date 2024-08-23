#ifndef CONIO_POPUP_H
#define	CONIO_POPUP_H

#include "pico.h"
#include "pico/stdlib.h"
#include "pico/scanvideo.h"
#include "pico/scanvideo/composable_scanline.h"

#define MAXIMUM_POPUPS_ALLOWED          3
#define DEFAULT_POPUP_DURATION_MS       750
#define POPUP_UPDATE_INTERVAL_MS        50

typedef struct {
    bool                popupAllocated;
    st_conioCharacter   previousRowContent[TEXT_COLUMNS];
    char                popupRowContent[TEXT_COLUMNS];
    uint16_t            remainingDisplayTime;
} st_conioPopup;

/* Popup functions */
void conio_initialisePopupBuffer(void);
uint8_t conio_displayPopup(char *string_p, e_colourPaletteIndexes foregroundColourIndex, e_colourPaletteIndexes backgroundColourIndex, uint16_t popupDuration);
void conio_updatePopupTask(void);

#endif // CONIO_POPUP_H