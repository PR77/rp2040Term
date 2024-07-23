#include "pico.h"
#include "pico/stdlib.h"
#include "string.h"
#include "time.h"
#include "tusb.h"
#include "class/hid/hid.h"
#include "keyboard.h"

static st_keyboardConfiguration keyboardConfiguration;
static st_deviceReports deviceReports;

const char* protocolStrings[] = { "None", "Keyboard", "Mouse" };

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
    tusb_init();
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
}

void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t __attribute__((unused)) instance) {

    if (dev_addr == keyboardConfiguration.keyboardAddress) {
        keyboardConfiguration.keyboardMounted = false;
        keyboardConfiguration.keyboardAddress = 0;
        keyboardConfiguration.deviceStr = (uint8_t *)protocolStrings[0];
    }
}

void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t __attribute__((unused)) instance, uint8_t const* report, uint16_t len) {
    
    if (dev_addr != keyboardConfiguration.keyboardAddress) {
        return;
    }

    uint8_t const rpt_count = deviceReports.deviceReportCount;
    tuh_hid_report_info_t* rpt_info_arr = deviceReports.deviceInformation;
    tuh_hid_report_info_t* rpt_info = NULL;

    if ( rpt_count == 1 && rpt_info_arr[0].report_id == 0) {
        
        // Simple report without report ID as 1st byte
        rpt_info = &rpt_info_arr[0];
    } else {
        // Composite report, 1st byte is report ID, data starts from 2nd byte
        uint8_t const rpt_id = report[0];

        // Find report id in the arrray
        for(uint8_t i=0; i<rpt_count; i++) {
            if (rpt_id == rpt_info_arr[i].report_id ) {
                rpt_info = &rpt_info_arr[i];
                break;
            }
        }

        report++;
        len--;
    }

    if (!rpt_info) {
        #ifdef DEBUG
        printf("Couldn't find the report info for this report !\r\n");
        #endif
        return;
    }

    if ( rpt_info->usage_page == HID_USAGE_PAGE_DESKTOP) {
        switch (rpt_info->usage) {
            case HID_USAGE_DESKTOP_KEYBOARD:
            // Assume keyboard follow boot report layout
            //process_kbd_report( (hid_keyboard_report_t const*) report );
            break;

            default:
            break;
        }
    }
}


/*
static uint8_t led_flags = 0;

void set_leds(uint8_t leds)
{
    uint8_t const addr = 1;
    led_flags = leds;

    tusb_control_request_t ledreq = {
        .bmRequestType_bit.recipient = TUSB_REQ_RCPT_INTERFACE,
        .bmRequestType_bit.type = TUSB_REQ_TYPE_CLASS,
        .bmRequestType_bit.direction = TUSB_DIR_OUT,
        .bRequest = HID_REQ_CONTROL_SET_REPORT,
        .wValue = HID_REPORT_TYPE_OUTPUT << 8,
        .wIndex = 0, // Interface number
        .wLength = sizeof(led_flags)};

    //tuh_control_xfer (addr, &ledreq, &led_flags, NULL);
}

void key_event(uint8_t key, bool bPress)
{
    uint8_t leds = led_flags;
    char sEvent[5];
#ifdef DEBUG
    printf("%s key 0x%02X\n", bPress ? "Press" : "Release", key);
#endif
    sprintf(sEvent, "%c%02hhX ", bPress ? 'P' : 'R', key);
    putstr(sEvent, bPress ? colours[12] : colours[3], colours[0]);
    if (bPress)
    {
        switch (key)
        {
        case HID_KEY_NUM_LOCK:
            leds ^= KEYBOARD_LED_NUMLOCK;
            set_leds(leds);
            break;
        case HID_KEY_CAPS_LOCK:
            leds ^= KEYBOARD_LED_CAPSLOCK;
            set_leds(leds);
            break;
        case HID_KEY_SCROLL_LOCK:
            leds ^= KEYBOARD_LED_SCROLLLOCK;
            set_leds(leds);
            break;
        default:
            break;
        }
    }
}

// look up new key in previous keys
static inline int find_key_in_report(hid_keyboard_report_t const *p_report, uint8_t keycode)
{
    for (int i = 0; i < 6; i++)
    {
        if (p_report->keycode[i] == keycode)
            return i;
    }
    return -1;
}

static inline void process_kbd_report(hid_keyboard_report_t const *p_new_report)
{
    static hid_keyboard_report_t prev_report = {0, 0, {0}}; // previous report to check key released
    bool held[6];
    for (int i = 0; i < 6; ++i)
        held[i] = false;
    for (int i = 0; i < 6; ++i)
    {
        uint8_t key = prev_report.keycode[i];
        if (key)
        {
            int kr = find_key_in_report(p_new_report, key);
            if (kr >= 0)
            {
                held[kr] = true;
            }
            else
            {
                key_event(key, false);
            }
        }
    }
    int old_mod = prev_report.modifier;
    int new_mod = p_new_report->modifier;
    int bit = 0x01;
    for (int i = 0; i < 8; ++i)
    {
        if ((old_mod & bit) && !(new_mod & bit))
            key_event(HID_KEY_CONTROL_LEFT + i, false);
        bit <<= 1;
    }
    bit = 0x01;
    for (int i = 0; i < 8; ++i)
    {
        if (!(old_mod & bit) && (new_mod & bit))
            key_event(HID_KEY_CONTROL_LEFT + i, true);
        bit <<= 1;
    }
    for (int i = 0; i < 6; ++i)
    {
        uint8_t key = p_new_report->keycode[i];
        if ((!held[i]) && (key))
        {
            key_event(key, true);
        }
    }

    prev_report = *p_new_report;
}

// Each HID instance can has multiple reports

#define MAX_REPORT  4
static uint8_t kbd_addr;
static uint8_t _report_count;
static tuh_hid_report_info_t _report_info_arr[MAX_REPORT];

//--------------------------------------------------------------------+
// TinyUSB Callbacks
//--------------------------------------------------------------------+

// Invoked when device with hid interface is mounted
// Report descriptor is also available for use. tuh_hid_parse_report_descriptor()
// can be used to parse common/simple enough descriptor.
void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* desc_report, uint16_t desc_len)
    {
    // Interface protocol
    uint8_t const interface_protocol = tuh_hid_interface_protocol(dev_addr, instance);
    if ( interface_protocol == HID_ITF_PROTOCOL_KEYBOARD )
        {
        kbd_addr = dev_addr;
#ifdef DEBUG
        printf ("Keyboard mounted: dev_addr = %d\n", dev_addr);
#endif
    
        _report_count = tuh_hid_parse_report_descriptor(_report_info_arr, MAX_REPORT,
            desc_report, desc_len);
#ifdef DEBUG
        printf ("%d reports defined\n", _report_count);
#endif
        putstr ("Ins ", colours[63], colours[0]);
        }
    }

// Invoked when device with hid interface is un-mounted
void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t __attribute__((unused)) instance)
    {
#ifdef DEBUG
    printf ("Device %d unmounted\n");
#endif
    if ( dev_addr == kbd_addr )
        {
        kbd_addr = 0;
        putstr ("Rem ", colours[63], colours[0]);
        }
    }

// Invoked when received report from device via interrupt endpoint
void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t __attribute__((unused)) instance,
    uint8_t const* report, uint16_t len)
    {
    if ( dev_addr != kbd_addr ) return;

    uint8_t const rpt_count = _report_count;
    tuh_hid_report_info_t* rpt_info_arr = _report_info_arr;
    tuh_hid_report_info_t* rpt_info = NULL;

    if ( rpt_count == 1 && rpt_info_arr[0].report_id == 0)
        {
        // Simple report without report ID as 1st byte
        rpt_info = &rpt_info_arr[0];
        }
    else
        {
        // Composite report, 1st byte is report ID, data starts from 2nd byte
        uint8_t const rpt_id = report[0];

        // Find report id in the arrray
        for(uint8_t i=0; i<rpt_count; i++)
            {
            if (rpt_id == rpt_info_arr[i].report_id )
                {
                rpt_info = &rpt_info_arr[i];
                break;
                }
            }

        report++;
        len--;
        }

    if (!rpt_info)
        {
#ifdef DEBUG
        printf("Couldn't find the report info for this report !\r\n");
#endif
        return;
        }

    if ( rpt_info->usage_page == HID_USAGE_PAGE_DESKTOP )
        {
        switch (rpt_info->usage)
            {
            case HID_USAGE_DESKTOP_KEYBOARD:
                // Assume keyboard follow boot report layout
                process_kbd_report( (hid_keyboard_report_t const*) report );
                break;

            default:
                break;
            }
        }
    }
}

void keyboard_loop(void)
{
    tusb_init();
    while (true)
    {
        tuh_task();
    }
}
*/
