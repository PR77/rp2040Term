#ifndef MAIN_H
#define	MAIN_H

#include "pico.h"
#include "pico/stdlib.h"
#include "bsp/board.h"

#define LCD_BACKLIGHTING_PWM_MIN    0
#define LCD_BACKLIGHTING_PWM_MAX    100

typedef struct {
    bool    beeper;
    uint8_t lcdBacklightValue;
} st_systemConfiguration;

st_systemConfiguration *system_getSystemConfiguration(void);

#endif // MAIN_H