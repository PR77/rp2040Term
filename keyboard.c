#include "pico.h"
#include "pico/stdlib.h"
#include "string.h"
#include "time.h"
#include "bsp/board.h"
#include "tusb.h"
#include "class/hid/hid.h"
#include "conio.h"
#include "keyboard.h"
#include "system.h"
#include "main.h"

static st_keyboardConfiguration keyboardConfiguration;
static st_deviceReports deviceReports;

static const char* protocolStrings[] = { "None", "Keyboard", "Mouse" };
static const uint8_t keycode2ascii[128][3] =  {DE_KEYCODE_TO_ASCII};

/**
    Initialise the USH Host and keyboard interface.
*/
void keyboard_initialiseKeyboard(void) {

    keyboardConfiguration.tusbInitialised = false;
    keyboardConfiguration.keyboardMounted = false;
    keyboardConfiguration.keyboardAddress = 0;
    keyboardConfiguration.deviceStr = (uint8_t *)protocolStrings[0];
    deviceReports.deviceReportCount = 0;
    memset(deviceReports.deviceInformation, 0, sizeof(tuh_hid_report_info_t));
    tuh_init(0);
}

/**
    Get a pointer to the keyboard configuration.

    @param[out]    st_keyboardConfiguration pointer to keyboard configuration structure.
*/
st_keyboardConfiguration *keyboard_getKeyboardConfiguration(void) {

    return (&keyboardConfiguration);
}

void keyboard_updateKeyboardTask(void) {

    if (true == tusb_inited()) {
        keyboardConfiguration.tusbInitialised = true;
        tuh_task();
    } else {
        keyboardConfiguration.tusbInitialised = false;
    }
}

void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* desc_report, uint16_t desc_len) {

    // Interface protocol (hid_interface_protocol_enum_t)
    uint8_t const itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);
    keyboardConfiguration.deviceStr = (uint8_t *)protocolStrings[itf_protocol];

    if (HID_ITF_PROTOCOL_KEYBOARD == itf_protocol) {
        keyboardConfiguration.keyboardMounted = true;
        keyboardConfiguration.keyboardAddress = dev_addr;
    }

    deviceReports.deviceReportCount = tuh_hid_parse_report_descriptor(deviceReports.deviceInformation, MAX_REPORT, desc_report, desc_len);

    // Request to receive report, tuh_hid_report_received_cb() will be invoked when report is available.
    // TODO: Error handling to be added if report can't be added.
    tuh_hid_receive_report(dev_addr, instance);
}

void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t __attribute__((unused)) instance) {

    if (dev_addr == keyboardConfiguration.keyboardAddress) {
        keyboardConfiguration.keyboardMounted = false;
        keyboardConfiguration.keyboardAddress = 0;
        keyboardConfiguration.deviceStr = (uint8_t *)protocolStrings[0];
    }
}

static inline bool find_key_in_report(hid_keyboard_report_t const *report, uint8_t keycode)
{
    for(uint8_t i=0; i<6; i++) {
        // Look up new key in previous keys
        if (report->keycode[i] == keycode) {
            return true;
        }
    }

    return false;
}

void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t __attribute__((unused)) instance, uint8_t const* report, uint16_t len) {

    uint8_t const itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);
     // previous reportS to check key released events.
    static hid_keyboard_report_t previousReport = { 0, 0, {0} };
    hid_keyboard_report_t const *currentReport = (hid_keyboard_report_t const *)report;

    if ((HID_ITF_PROTOCOL_KEYBOARD == itf_protocol) && 
        ((keyboardConfiguration.keyboardMounted == true) && (keyboardConfiguration.keyboardAddress == dev_addr))) { 

        for(uint8_t i = 0; i < 6; i++) {
            if (currentReport->keycode[i] != 0) {
                if ( find_key_in_report(&previousReport, currentReport->keycode[i]) ) {
                    // exist in previous report means the current key is holding
                } else {
                    // not existed in previous report means the current key is pressed
                    switch (currentReport->keycode[i]) {
                        case HID_KEY_F10:
                            system_decreaseBacklightByStep();
                            break;

                        case HID_KEY_F12:
                            system_increaseBacklightByStep();
                            break;

                        default: {
                            bool const is_shift = currentReport->modifier & (KEYBOARD_MODIFIER_LEFTSHIFT | KEYBOARD_MODIFIER_RIGHTSHIFT);
                            bool const is_altgrp = currentReport->modifier & (KEYBOARD_MODIFIER_RIGHTALT);
                            uint8_t ch = keycode2ascii[currentReport->keycode[i]][is_altgrp ? 2 : is_shift ? 1 : 0];
                            conio_printCharacter(ch, 1, 0);
                        }
                        break;
                    }
                }
            }
        }

        previousReport = *currentReport;
    }

    // Consume the report.
    // TODO: Error handling to be added if report can't be added.
    tuh_hid_receive_report(dev_addr, instance);
}
