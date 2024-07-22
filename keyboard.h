#ifndef KEYBOARD_H
#define	KEYBOARD_H

#include "pico.h"
#include "pico/stdlib.h"
#include "bsp/board.h"

typedef struct {
    bool        tusbInitialised;
    bool        deviceMounted;
    uint8_t     deviceAddress;
    uint8_t     *deviceStr;
} st_keyboardConfiguration;

void keyboard_initialiseKeyboard(void);
st_keyboardConfiguration *keyboard_getKeyboardConfiguration(void);
void keyboard_updateKeyboardTask(void);

#endif // KEYBOARD_H