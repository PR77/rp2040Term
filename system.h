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

#define VIDEO_RENDERING_CORE        1

/* Information on LCD Pixel Clock
.clock_freq = 9500000, // can only be used with set_sys_clock_pll (1482000000, 6, 2) results in a of clock of 123.5MHz;
.clock_freq = 7812500, // can only be used with set_sys_clock_khz (125000, true) results in a of clock of 125.9MHz;
*/

#define LCD_PIXEL_CLK               9500000

typedef struct {
    bool    beeper;
    bool    localEcho;
    uint8_t lcdBacklightValue;
} st_systemConfiguration;

extern const scanvideo_mode_t tftLQ042_480x272_60;

void system_initialiseSystem(void);
st_systemConfiguration *system_getSystemConfiguration(void);
void system_toggleBeeper(void);
void system_toggleLocalEcho(void);
void system_increaseBacklightByStep(void);
void system_decreaseBacklightByStep(void);
void system_onPwmWrap(void);
void __time_critical_func(system_renderLoop)(void);


#endif // SYSTEM_H