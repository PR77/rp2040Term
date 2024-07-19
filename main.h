#ifndef MAIN_H
#define	MAIN_H

#include "pico.h"
#include "pico/stdlib.h"
#include "bsp/board.h"

typedef struct {
    bool    beeper;
    uint8_t lcdBacklightValue;
} st_systemConfiguration;

st_systemConfiguration *system_getSystemConfiguration(void);

#endif // MAIN_H