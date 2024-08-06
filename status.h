#ifndef STATUS_H
#define	STATUS_H

#include "pico.h"
#include "pico/stdlib.h"
#include "pico/scanvideo.h"
#include "pico/scanvideo/composable_scanline.h"

#define STATUS_BAR_UPDATE_INTERVAL_MS   100

void status_initialiseStatusBar(e_colourPaletteIndexes foregroundColourIndex, e_colourPaletteIndexes backgroundColourIndex, bool invertText);

/* Status bar functions */
void status_forceStatusBarUpdate(void);
void status_updateStatusBarTask(void);

#endif // STATUS_H