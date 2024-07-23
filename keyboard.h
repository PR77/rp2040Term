#ifndef KEYBOARD_H
#define	KEYBOARD_H

#include "pico.h"
#include "pico/stdlib.h"
#include "bsp/board.h"

#define MAX_REPORT  4

typedef struct {
    bool                    tusbInitialised;
    bool                    keyboardMounted;
    uint8_t                 keyboardAddress;
    uint8_t                 *deviceStr;
} st_keyboardConfiguration;

typedef struct {
    uint8_t                 deviceReportCount;
    tuh_hid_report_info_t   deviceInformation[MAX_REPORT];
} st_deviceReports;

void keyboard_initialiseKeyboard(void);
st_keyboardConfiguration *keyboard_getKeyboardConfiguration(void);
void keyboard_updateKeyboardTask(void);

#endif // KEYBOARD_H