#ifndef KEYBOARD_H
#define	KEYBOARD_H

#include "pico.h"
#include "pico/stdlib.h"
#include "bsp/board.h"

#define LOCALISE_DE
#define MAX_REPORT              4
#define MAX_CUSTOM_KEY_HANDLERS 6

//--------------------------------------------------------------------/
 // DE keyboard
 //
 // third column is ALT GR modification
 //--------------------------------------------------------------------/

 #ifdef LOCALISE_DE
 #define KEYMAP "DE"
 #define KEYMAP_REV 2
 #define DE_KEYCODE_TO_ASCII    \
    {0     , 0     ,0   }, /* 0x00 */ \
    {0     , 0     ,0   }, /* 0x01 */ \
    {0     , 0     ,0   }, /* 0x02 */ \
    {0     , 0     ,0   }, /* 0x03 */ \
    {'a'   , 'A'   ,0   }, /* 0x04 */ \
    {'b'   , 'B'   ,0   }, /* 0x05 */ \
    {'c'   , 'C'   ,0   }, /* 0x06 */ \
    {'d'   , 'D'   ,0   }, /* 0x07 */ \
    {'e'   , 'E'   ,0   }, /* 0x08 */ \
    {'f'   , 'F'   ,0   }, /* 0x09 */ \
    {'g'   , 'G'   ,0   }, /* 0x0a */ \
    {'h'   , 'H'   ,0   }, /* 0x0b */ \
    {'i'   , 'I'   ,0   }, /* 0x0c */ \
    {'j'   , 'J'   ,0   }, /* 0x0d */ \
    {'k'   , 'K'   ,0   }, /* 0x0e */ \
    {'l'   , 'L'   ,0   }, /* 0x0f */ \
    {'m'   , 'M'   ,0   }, /* 0x10 */ \
    {'n'   , 'N'   ,0   }, /* 0x11 */ \
    {'o'   , 'O'   ,0   }, /* 0x12 */ \
    {'p'   , 'P'   ,0   }, /* 0x13 */ \
    {'q'   , 'Q'   ,'@' }, /* 0x14 */ \
    {'r'   , 'R'   ,0   }, /* 0x15 */ \
    {'s'   , 'S'   ,0   }, /* 0x16 */ \
    {'t'   , 'T'   ,0   }, /* 0x17 */ \
    {'u'   , 'U'   ,0   }, /* 0x18 */ \
    {'v'   , 'V'   ,0   }, /* 0x19 */ \
    {'w'   , 'W'   ,0   }, /* 0x1a */ \
    {'x'   , 'X'   ,0   }, /* 0x1b */ \
    {'z'   , 'Z'   ,0   }, /* 0x1c */ \
    {'y'   , 'Y'   ,0   }, /* 0x1d */ \
    {'1'   , '!'   ,0   }, /* 0x1e */ \
    {'2'   , '\"'  ,0   }, /* 0x1f */ \
    {'3'   , 0     ,0   }, /* 0x20 */ \
    {'4'   , '$'   ,0   }, /* 0x21 */ \
    {'5'   , '%'   ,0   }, /* 0x22 */ \
    {'6'   , '&'   ,0   }, /* 0x23 */ \
    {'7'   , '/'   ,'{' }, /* 0x24 */ \
    {'8'   , '('   ,'[' }, /* 0x25 */ \
    {'9'   , ')'   ,']' }, /* 0x26 */ \
    {'0'   , '='   ,'}' }, /* 0x27 */ \
    {'\r'  , '\r'  ,0   }, /* 0x28 */ \
    {'\x1b', '\x1b',0   }, /* 0x29 */ \
    {'\b'  , '\b'  ,0   }, /* 0x2a  Backspace */ \
    {'\t'  , '\t'  ,0   }, /* 0x2b */ \
    {' '   , ' '   ,0   }, /* 0x2c */ \
    { 0    , '?'   ,'\\'}, /* 0x2d  ß */ \
    {','   , '\''  ,0   }, /* 0x2e */ \
    { 0    , 0     ,0   }, /* 0x2f */ \
    {'+'   , '*'   ,'~' }, /* 0x30 */ \
    {'#'   , '\''  ,0   }, /* 0x31 */ \
    {'#'   , '\''  ,0   }, /* 0x32 */ \
    { 0    , 0     ,0   }, /* 0x33 */ \
    { 0    , 0     ,0   }, /* 0x34 */ \
    {'`'   , '~'   ,0   }, /* 0x35 */ \
    {','   , ';'   ,0   }, /* 0x36 */ \
    {'.'   , ':'   ,0   }, /* 0x37 */ \
    {'-'   , '_'   ,0   }, /* 0x38 */ \
    {0     , 0     ,0   }, /* 0x39 */ \
    {0     , 0     ,0   }, /* 0x3a */ \
    {0     , 0     ,0   }, /* 0x3b */ \
    {0     , 0     ,0   }, /* 0x3c */ \
    {0     , 0     ,0   }, /* 0x3d */ \
    {0     , 0     ,0   }, /* 0x3e */ \
    {0     , 0     ,0   }, /* 0x3f */ \
    {0     , 0     ,0   }, /* 0x40 */ \
    {0     , 0     ,0   }, /* 0x41 */ \
    {0     , 0     ,0   }, /* 0x42 */ \
    {0     , 0     ,0   }, /* 0x43 */ \
    {0     , 0     ,0   }, /* 0x44 */ \
    {0     , 0     ,0   }, /* 0x45 */ \
    {0     , 0     ,0   }, /* 0x46 */ \
    {0     , 0     ,0   }, /* 0x47 */ \
    {0     , 0     ,0   }, /* 0x48 */ \
    {0x16  , 0     ,0   }, /* 0x49 INS   */ \
    {0     , 0     ,0   }, /* 0x4a       */ \
    {0x12  , 0     ,0   }, /* 0x4b PGUP  */ \
    {0x7f  , 0     ,0   }, /* 0x4c DEL   */ \
    {0     , 0     ,0   }, /* 0x4d       */ \
    {0x03  , 0     ,0   }, /* 0x4e PGDN  */ \
    {0x04  , 0     ,0   }, /* 0x4f RIGHT */ \
    {0x13  , 0     ,0   }, /* 0x50 LEFT  */ \
    {0x18  , 0     ,0   }, /* 0x51 DOWN  */ \
    {0x05  , 0     ,0   }, /* 0x52 UP    */ \
    {0     , 0     ,0   }, /* 0x53       */ \
    {'/'   , '/'   ,0   }, /* 0x54 */ \
    {'*'   , '*'   ,0   }, /* 0x55 */ \
    {'-'   , '-'   ,0   }, /* 0x56 */ \
    {'+'   , '+'   ,0   }, /* 0x57 */ \
    {'\r'  , '\r'  ,0   }, /* 0x58 */ \
    {'1'   , 0     ,0   }, /* 0x59 */ \
    {'2'   , 0     ,0   }, /* 0x5a */ \
    {'3'   , 0     ,0   }, /* 0x5b */ \
    {'4'   , 0     ,0   }, /* 0x5c */ \
    {'5'   , '5'   ,0   }, /* 0x5d */ \
    {'6'   , 0     ,0   }, /* 0x5e */ \
    {'7'   , 0     ,0   }, /* 0x5f */ \
    {'8'   , 0     ,0   }, /* 0x60 */ \
    {'9'   , 0     ,0   }, /* 0x61 */ \
    {'0'   , 0     ,0   }, /* 0x62 */ \
    {'.'   , 0     ,0   }, /* 0x63 */ \
    {'<'  , '>'   ,'|'  }, /* 0x64 */ \
    {'0'   , 0     ,0   }, /* 0x65 */ \
    {'0'   , 0     ,0   }, /* 0x66 */ \
    {'='   , '='   ,0   }, /* 0x67 */ \

 #endif

typedef struct {
    bool                    tusbInitialised;
    bool                    keyboardMounted;
    uint8_t                 keyboardAddress;
    uint8_t                 *deviceStr;
} st_keyboardConfiguration;

typedef struct {
    uint8_t                 deviceReportCount;
    tuh_hid_report_info_t   deviceInformation[MAX_REPORT];
} st_keyboardDeviceReports;

typedef struct {
    void                    (* keyPressedHandler)(void);
} st_keyboardSystemResetHandler;

typedef struct {
    void                    (* keyPressedHandler)(uint8_t keyCharacter);
    void                    (* keyReleasedHandler)(uint8_t keyCharacter);
} st_keyboardDefaultHandler;

typedef struct {
    uint8_t                 hidKeyCode;
    void                    (* keyPressedHandler)(void);
    void                    (* keyReleasedHandler)(void);
} st_keyboardCustomHandler;

void keyboard_initialiseKeyboard(void);
st_keyboardConfiguration *keyboard_getKeyboardConfiguration(void);
bool keyboard_attachDefaultKeyHandler(void (*keyPressedHandler)(uint8_t keyCharacter), void (*keyReleasedHandler)(uint8_t keyCharacter));
bool keyboard_attachCustomKeyHandler(uint hidKeyCode, void (*keyPressedHandler)(void), void (*keyReleasedHandler)(void));
bool keyboard_attachSystemResetHandler(void (*keyPressedHandler)(void));
void keyboard_updateKeyboardTask(void);

#endif // KEYBOARD_H