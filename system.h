#ifndef SYSTEM_H
#define	SYSTEM_H

#include "pico.h"
#include "pico/stdlib.h"
#include "bsp/board.h"

#define LCD_BACKLIGHT_PWM_PIN       28

#define LCD_BACKLIGHTING_PWM_MIN    0
#define LCD_BACKLIGHTING_PWM_MAX    100
#define LCD_BACKLIGHTING_STEP_SIZE  10
#define LCD_BACKLIGHTING_DEFAULT    70

typedef struct {
    bool    beeper;
    uint8_t lcdBacklightValue;
} st_systemConfiguration;

void system_initialiseSystem(void);
st_systemConfiguration *system_getSystemConfiguration(void);
void system_toggleBeeper(void);
void system_increaseBacklightByStep(void);
void system_decreaseBacklightByStep(void);
void system_onPwmWrap(void);

#endif // SYSTEM_H