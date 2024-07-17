/*  main.c - The main program -------------------------------------------------
__________ __________      _________________________________
\______   \\______   \     \__    ___/\_   _____/\__    ___/
 |       _/ |     ___/       |    |    |    __)    |    |   
 |    |   \ |    |           |    |    |     \     |    |   
 |____|_  / |____|     /\    |____|    \___  /     |____|   
        \/             \/                  \/               
https://patorjk.com/software/taag/#p=display&h=0&v=0&f=Graffiti

RP2040 Terminal Emulator for parallel RBG666 TFT Display using the RP2040
scanvideo library. I used bits-and-pieces for these awesome repos to develop
this;

https://geoffg.net/terminal.html
https://github.com/Memotech-Bill/pico-colour-text/tree/main
https://github.com/RC2014Z80/picoterm

-----------------------------------------------------------------------------*/

#include "pico.h"
#include "pico/stdlib.h"
#include "pico/scanvideo.h"
#include "pico/scanvideo/composable_scanline.h"
#include "pico/multicore.h"
#include "hardware/clocks.h"
#include "time.h"
#include "bsp/board.h"
#include "tusb.h"
#include "class/hid/hid.h"
#include "font_sun8x16.h"
#include "conio.h"

#define VID_CORE                1
#define LED_FLASH_INTERVAL_MS   500

static semaphore_t videoInitialised;

void __time_critical_func(renderLoop)(void) {
    
    int core_num = get_core_num();
    assert (core_num >= 0 && core_num < 2);

    while (true)
    {
        struct scanvideo_scanline_buffer *buffer = scanvideo_begin_scanline_generation(true);
        uint16_t *pix = (uint16_t *)buffer->data;
        uint16_t scanlineNumber = scanvideo_scanline_number(buffer->scanline_id);
        uint8_t rowIndex = (scanlineNumber / GLYPH_HEIGHT);
        uint8_t rowScanline = (scanlineNumber % GLYPH_HEIGHT);
        
        if (rowIndex > TEXT_ROWS) {
            rowIndex -= TEXT_ROWS + 1;
        }

        pix += 2;
        for (uint8_t columnIndex = 0; columnIndex < TEXT_COLUMNS; ++columnIndex)
        {
            // In this loop all glyph parts for specific scanline are rendered.
            st_conioCharacter *ch = conio_getCharacterBuffer(rowIndex, columnIndex);
            uint16_t foregroundColour = ch->foregroundColour;
            uint16_t backgroundColour = ch->backgroundColour;
            bool invert = ch->invert;
            uint8_t fontBits = fontdata_sun8x16[ch->locationCharacter][rowScanline];

            if (invert == true) {
                fontBits = ~fontBits;
            }

            pix[0] = (fontBits & 0x80) ? foregroundColour : backgroundColour;
            pix[1] = (fontBits & 0x40) ? foregroundColour : backgroundColour;
            pix[2] = (fontBits & 0x20) ? foregroundColour : backgroundColour;
            pix[3] = (fontBits & 0x10) ? foregroundColour : backgroundColour;
            pix[4] = (fontBits & 0x08) ? foregroundColour : backgroundColour;
            pix[5] = (fontBits & 0x04) ? foregroundColour : backgroundColour;
            pix[6] = (fontBits & 0x02) ? foregroundColour : backgroundColour;
            pix[7] = (fontBits & 0x01) ? foregroundColour : backgroundColour;
            pix += GLYPH_WIDTH;
        }

        ++pix;
        *pix = 0;
        ++pix;
        *pix = COMPOSABLE_EOL_ALIGN;
        
        pix = (uint16_t *)buffer->data;
        pix[0] = COMPOSABLE_RAW_RUN;
        pix[1] = pix[2];
        pix[2] = TEXT_COLUMNS * GLYPH_WIDTH - 2;
        buffer->data_used = (TEXT_COLUMNS * GLYPH_WIDTH + 4) / 2;
        assert(buffer->data_used < buffer->data_max);

        scanvideo_end_scanline_generation(buffer);
    } // end while(true) loop
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

void keyboard_loop(void)
{
    tusb_init();
    while (true)
    {
        tuh_task();
    }
}
*/

void system_initialiseScanVideo(void) {

    scanvideo_setup(&vga_mode_480x272_60);
    scanvideo_timing_enable(true);
    sem_release(&videoInitialised);
    renderLoop();
}

void system_updateLedTask() {
    static uint64_t previousTime = 0;
    static bool currentLedState = false;

    // Blink every interval ms
    if ((to_ms_since_boot(get_absolute_time()) - previousTime) > LED_FLASH_INTERVAL_MS) {
        previousTime += LED_FLASH_INTERVAL_MS;
        
        gpio_put(PICO_DEFAULT_LED_PIN, currentLedState);
        currentLedState ^= true;
    }
}

int main(void) {

    uint8_t msgBuffer[TEXT_COLUMNS];

    set_sys_clock_khz(125000, true);

    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_LED_PIN, false);

    conio_initialiseCharacterBuffer(PALETTE_COLOUR_AMBER_INDEX, PALETTE_COLOUR_BLACK_INDEX);

    // create a semaphore to be posted when video init is complete
    sem_init(&videoInitialised, 0, 1);
    multicore_launch_core1(system_initialiseScanVideo);
    // wait for initialization of video to be complete
    sem_acquire_blocking(&videoInitialised);

	//                 012345678901234567890123456789012345678901234567890123456789
	//                 |        |         |         |         |         |         |
    conio_printString("--------------------------------[RP2040Term TFT - By PR77]--\r\n", PALETTE_COLOUR_YELLOW_INDEX, PALETTE_COLOUR_BLACK_INDEX);
    conio_printString("__________ __________      _________________________________\r\n", PALETTE_COLOUR_AMBER_INDEX, PALETTE_COLOUR_BLACK_INDEX);
    conio_printString("\\______   \\\\______   \\     \\__    ___/\\_   _____/\\__    ___/\r\n", PALETTE_COLOUR_AMBER_INDEX, PALETTE_COLOUR_BLACK_INDEX);
    conio_printString(" |       _/ |     ___/       |    |    |    __)    |    |\r\n", PALETTE_COLOUR_AMBER_INDEX, PALETTE_COLOUR_BLACK_INDEX);   
    conio_printString(" |    |   \\ |    |           |    |    |     \\     |    |\r\n", PALETTE_COLOUR_AMBER_INDEX, PALETTE_COLOUR_BLACK_INDEX);   
    conio_printString(" |____|_  / |____|     /\\    |____|    \\___  /     |____|\r\n", PALETTE_COLOUR_AMBER_INDEX, PALETTE_COLOUR_BLACK_INDEX);   
    conio_printString("        \\/             \\/                  \\/\r\n", PALETTE_COLOUR_AMBER_INDEX, PALETTE_COLOUR_BLACK_INDEX);               

	//                 012345678901234567890123456789012345678901234567890123456789
	//                 |        |         |         |         |         |         |
    conio_printString("--[GPL 3 Clause]--------------------------------[(c) 2024]--\r\n", PALETTE_COLOUR_WHITE_INDEX, PALETTE_COLOUR_BLACK_INDEX);
    conio_printString("\r\n", PALETTE_COLOUR_WHITE_INDEX, PALETTE_COLOUR_BLACK_INDEX);
    //                 012345678901234567890123456789012345678901234567890123456789
	//                 |        |         |         |         |         |         |
    snprintf(msgBuffer, TEXT_COLUMNS, "Version %s, Build %s, Release %s\r\n", CMAKE_PROJECT_VERSION, __DATE__, CMAKE_PROJECT_DESCRIPTION);
    conio_printString(msgBuffer, PALETTE_COLOUR_WHITE_INDEX, PALETTE_COLOUR_BLACK_INDEX);
    conio_enableCursor();
    /*
    keyboard_loop();
    */
    while (true) {
        system_updateLedTask();
        conio_updateCursorTask();
    }
}
