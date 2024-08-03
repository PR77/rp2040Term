#include "pico.h"
#include "pico/stdlib.h"
#include "string.h"
#include "tusb.h"
#include "class/hid/hid.h"
#include "keyboard.h"

static st_keyboardConfiguration keyboardConfiguration;
static st_keyboardDeviceReports keyboardDeviceReports;
static st_keyboardDefaultHandler keyboardDefaultHandler;
static st_keyboardSystemResetHandler keyboardSystemResetHandler;
static st_keyboardCustomHandler keyboardCustomHandlers[MAX_CUSTOM_KEY_HANDLERS];

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
    keyboardDeviceReports.deviceReportCount = 0;
    memset(keyboardDeviceReports.deviceInformation, 0, sizeof(tuh_hid_report_info_t));
    memset(&keyboardDefaultHandler, 0, sizeof(st_keyboardDefaultHandler));
    memset(keyboardCustomHandlers, 0, sizeof(st_keyboardCustomHandler) * MAX_CUSTOM_KEY_HANDLERS);
    memset(&keyboardSystemResetHandler, 0, sizeof(st_keyboardSystemResetHandler));
    tuh_init(0);
}

/**
    Get a pointer to the keyboard configuration.

    @return[out]    st_keyboardConfiguration pointer to keyboard configuration structure.
*/
st_keyboardConfiguration *keyboard_getKeyboardConfiguration(void) {

    return (&keyboardConfiguration);
}

/**
    Attach a default key handler for presses and releases.

    @param[in]     keyPressedHandler function pointer to key press handler. Can be NULL if not required.
    @param[in]     keyReleasedHandler function pointer to key release handler. Can be NULL if not required.
    @returns[out]  bool true if attach was successful, otherwise false.
*/
bool keyboard_attachDefaultKeyHandler(void (*keyPressedHandler)(uint8_t keyCharacter), void (*keyReleasedHandler)(uint8_t keyCharacter)) {
    
    bool attachedSuccess = false;

    if ((*keyPressedHandler != NULL) || (*keyReleasedHandler != NULL)) {
        keyboardDefaultHandler.keyPressedHandler = keyPressedHandler;
        keyboardDefaultHandler.keyReleasedHandler = keyReleasedHandler;
        attachedSuccess = true;
    }

    return (attachedSuccess);
}

/**
    Attach a custom key handler for presses and releases. A total of MAX_CUSTOM_KEY_HANDLERS
    can be added.

    @param[in]     hidKeyCode HID key code to map customer handler to.
    @param[in]     keyPressedHandler function pointer to key press handler. Can be NULL if not required.
    @param[in]     keyReleasedHandler function pointer to key release handler. Can be NULL if not required.
    @return[out]   bool true if attach was successful, otherwise false.
*/
bool keyboard_attachCustomKeyHandler(uint hidKeyCode, void (*keyPressedHandler)(void), void (*keyReleasedHandler)(void)) {

    bool attachedSuccess = false;

    if (HID_KEY_NONE != hidKeyCode) {
        for (uint8_t i = 0; i < MAX_CUSTOM_KEY_HANDLERS; i++) {
            if (keyboardCustomHandlers[i].hidKeyCode == HID_KEY_NONE) {
                // Found an empty location, lets populate it with the handlers.
                keyboardCustomHandlers[i].hidKeyCode = hidKeyCode;
                keyboardCustomHandlers[i].keyPressedHandler = keyPressedHandler;
                keyboardCustomHandlers[i].keyReleasedHandler = keyReleasedHandler;
                attachedSuccess = true;
                break;
            }
        }
    }

    return (attachedSuccess);
}

/**
    Attach a custom reset handler executing system resets via CTRL-ALT-DEL combination.
    
    @param[in]     keyPressedHandler function pointer to system reset handler. Can be NULL if not required.
    @return[out]   bool true if attach was successful, otherwise false.
*/
bool keyboard_attachSystemResetHandler(void (*keyPressedHandler)(void)) {

    bool attachedSuccess = false;

    if (*keyPressedHandler != NULL) {
        keyboardSystemResetHandler.keyPressedHandler = keyPressedHandler;
        attachedSuccess = true;
    }

    return (attachedSuccess);
}

/**
    Cyclic function to handle keyboard and USB handling.
*/
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

    keyboardDeviceReports.deviceReportCount = tuh_hid_parse_report_descriptor(keyboardDeviceReports.deviceInformation, MAX_REPORT, desc_report, desc_len);

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

static inline bool findCustomKeyInReport(uint8_t keycode, uint8_t *index) {

    for (uint8_t i = 0; i < MAX_CUSTOM_KEY_HANDLERS; i++) {
        if (keyboardCustomHandlers[i].hidKeyCode == keycode) {
            *index = i;
            return true;
        }
    }

    return false;
}

void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t __attribute__((unused)) instance, uint8_t const* report, uint16_t len) {

    uint8_t const itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);
    // Previous reports to check key released events.
    static hid_keyboard_report_t previousReport = { 0, 0, {0} };
    hid_keyboard_report_t const *currentReport = (hid_keyboard_report_t const *)report;

    if ((HID_ITF_PROTOCOL_KEYBOARD == itf_protocol) && 
        ((keyboardConfiguration.keyboardMounted == true) && (keyboardConfiguration.keyboardAddress == dev_addr))) { 

        for(uint8_t i = 0; i < 6; i++) {
            if (currentReport->keycode[i] != 0) {
                // If keycode in current report is != 0 current key is pressed.
                uint8_t customerHandlerIndex = 0;
                if (true == findCustomKeyInReport(currentReport->keycode[i], &customerHandlerIndex)) {
                    if (NULL != keyboardCustomHandlers[customerHandlerIndex].keyPressedHandler) {
                        keyboardCustomHandlers[customerHandlerIndex].keyPressedHandler();
                    }
                } else {
                    bool const is_shift = currentReport->modifier & (KEYBOARD_MODIFIER_LEFTSHIFT | KEYBOARD_MODIFIER_RIGHTSHIFT);
                    bool const is_altgrp = currentReport->modifier & (KEYBOARD_MODIFIER_RIGHTALT);
                    bool const is_ctrl = currentReport->modifier & (KEYBOARD_MODIFIER_LEFTCTRL);
                    bool const is_leftalt = currentReport->modifier & (KEYBOARD_MODIFIER_LEFTALT);
                    uint8_t keyCharacter = keycode2ascii[currentReport->keycode[i]][is_altgrp ? 2 : is_shift ? 1 : 0];

                    if ((is_ctrl == true) && (is_leftalt == true) && (currentReport->keycode[i] == HID_KEY_DELETE)) {
                        // Three finger salute detected, lets call whatever has been attached.
                        if (NULL != keyboardSystemResetHandler.keyPressedHandler) {
                            keyboardSystemResetHandler.keyPressedHandler();
                        }
                    }

                    // Check if there is a default key handler and call it if assigned.
                    if (NULL != keyboardDefaultHandler.keyPressedHandler) {
                        keyboardDefaultHandler.keyPressedHandler(keyCharacter);
                    }
                }
            } else if (previousReport.keycode[i] != 0) {
                // ... else if keycode in previous report is != 0 current key is released.
                uint8_t customerHandlerIndex = 0;

                if (true == findCustomKeyInReport(previousReport.keycode[i], &customerHandlerIndex)) {
                    if (NULL != keyboardCustomHandlers[customerHandlerIndex].keyReleasedHandler) {
                        keyboardCustomHandlers[customerHandlerIndex].keyReleasedHandler();
                    }
                } else {
                    if (NULL != keyboardDefaultHandler.keyReleasedHandler) {
                        bool const is_shift = currentReport->modifier & (KEYBOARD_MODIFIER_LEFTSHIFT | KEYBOARD_MODIFIER_RIGHTSHIFT);
                        bool const is_altgrp = currentReport->modifier & (KEYBOARD_MODIFIER_RIGHTALT);
                        uint8_t keyCharacter = keycode2ascii[previousReport.keycode[i]][is_altgrp ? 2 : is_shift ? 1 : 0];

                        keyboardDefaultHandler.keyReleasedHandler(keyCharacter);
                    }
                }
            } else {
                // ... Do nothing
            }
        }

        previousReport = *currentReport;
    }

    // Consume the report.
    // TODO: Error handling to be added if report can't be added.
    tuh_hid_receive_report(dev_addr, instance);
}