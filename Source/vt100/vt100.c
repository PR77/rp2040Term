/****************************************************************************************
 vt100.c

    vt100 decoder for the VT100 Terminal program


	Copyright (C) 2014 Geoff Graham (projects@geoffg.net)
	All rights reserved.

	This file and the program created from it are FREE FOR COMMERCIAL AND
	NON-COMMERCIAL USE as long as the following conditions are aheared to.

	Copyright remains Geoff Graham's, and as such any Copyright notices in the
	code are not to be removed.  If this code is used in a product, Geoff Graham
	should be given attribution as the author of the parts used.  This can be in
	the form of a textual message at program startup or in documentation (online
	or textual) provided with the program or product.

	Redistribution and use in source and binary forms, with or without modification,
	are permitted provided that the following conditions  are met:
	1. Redistributions of source code must retain the copyright notice, this list
	   of conditions and the following disclaimer.
	2. Redistributions in binary form must reproduce the above copyright notice, this
	   list of conditions and the following disclaimer in the documentation and/or
	   other materials provided with the distribution.
	3. All advertising materials mentioning features or use of this software must
	   display the following acknowledgement:
	   This product includes software developed by Geoff Graham (projects@geoffg.net)

	THIS SOFTWARE IS PROVIDED BY GEOFF GRAHAM ``AS IS'' AND  ANY EXPRESS OR IMPLIED
	WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
	MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT
	SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
	SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
	PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
	BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
	CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
	IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
	SUCH DAMAGE.

	The licence and distribution terms for any publically available version or
	derivative of this code cannot be changed.  i.e. this code cannot simply be copied
	and put under another distribution licence (including the GNU Public Licence).
****************************************************************************************/
/****************************************************************************************
 *  Modifed By PR77 as part of the rp2040Term
****************************************************************************************/

#include "pico.h"
#include "pico/stdlib.h"
#include "string.h"
#include "ctype.h"
#include "time.h"
#include "pico/scanvideo.h"
#include "pico/scanvideo/composable_scanline.h"
#include "system.h"
#include "serial.h"
#include "conio.h"
#include "cursor.h"
#include "popup.h"
#include "vt100.h"

#define VTBUF_SIZE  40

static char vtbuf[VTBUF_SIZE + 1];   // buffer for chars waiting to be decoded
static uint8_t vtcnt;                   // count of the number of chars in vtbuf
static uint8_t arg[8], argc;            // arguments to a command
static uint8_t mode = VT100;

/***************************************************************************************************************************************
 The command table
 This is scanned from top to bottom by VT100Putc()
 Characters are matched exactly (ie, case sensitive) except for the @ character which is a wild card character representing
 zero or more decimal characters of an argument and ^ which represents a single char and the value returned is the char - 31
 **************************************************************************************************************************************/
const st_vt100CommandTable vt100CommandTable[] = {
    { "\033[K",         VT100,      cmd_ClearEOL },
    { "\033[J",         VT100,      cmd_ClearEOS },
    { "\033[0K",        VT100,      cmd_ClearEOL },
    { "\033[1K",        VT100,      cmd_ClearBOL },
    { "\033[2K",        VT100,      cmd_ClearLine },
    { "\033[0J",        VT100,      cmd_ClearEOS },
    { "\033[1J",        VT100,      cmd_ClearBOS },
    { "\033[2J",        VT100,      cmd_ClearScreen },

    { "\033[?25l",      VT100,      cmd_CursorOff },
    { "\033[?25h",      VT100,      cmd_CursorOn },

    { "\033[@A",        VT100,      cmd_CurUp },
    { "\033[@B",        VT100,      cmd_CurDown },
    { "\033[@C",        VT100,      cmd_CurRight },
    { "\033[@D",        VT100,      cmd_CurLeft },
    { "\033[@;@H",      VT100,      cmd_CurPosition },
    { "\033[@;@f",      VT100,      cmd_CurPosition },
    { "\033[H",         VT100,      cmd_CurHome },
    { "\033[f",         VT100,      cmd_CurHome },

    { "\033D",          VT100,      cmd_LineFeed },

    { "\033c" ,         BOTH,       cmd_Reset },
    { "\033[c" ,        VT100,      cmd_VT100ID },
    { "\033[0c" ,       VT100,      cmd_VT100ID },
    { "\033[5n" ,       VT100,      cmd_VT100OK },
};

void vt100_putCharacter(char character) {
    int cmd, i, j, partial;

    if (vtcnt >= VTBUF_SIZE) {
        return;
    }

    vtbuf[vtcnt++] = character;
    partial = false;

    while (vtcnt) {
        for (cmd = 0; cmd < (sizeof(vt100CommandTable) / sizeof(vt100CommandTable[0])); cmd++) {
            if ((vt100CommandTable[cmd].mode & mode) && *vt100CommandTable[cmd].name == *vtbuf) {   // check the mode then a quick check of the first char
                arg[0] = argc = 0;
                for (j = i = 1; vt100CommandTable[cmd].name[i] && j < vtcnt; i++, j++) {
                    if (vt100CommandTable[cmd].name[i] == '^') {
                        arg[argc++] = vtbuf[j] - 31;
                    } else if (vt100CommandTable[cmd].name[i] == '@') {
                        arg[argc] = 0;
                        while (isdigit(vtbuf[j]))
                            arg[argc] = arg[argc] * 10 + (vtbuf[j++] - '0');
                        j--;                                        // correct for the overshoot, so that the next loop looks at the next char
                        argc++;
                    } else if (vt100CommandTable[cmd].name[i] != vtbuf[j]) {
                        goto nextcmd;                           // compare failed, try the next command
                    }
                }

                if (vt100CommandTable[cmd].name[i] == 0) {                      // compare succeded, we have found the command
                    vtcnt = 0;                                      // clear all chars in the queue
                    vt100CommandTable[cmd].commandHandler();                             // and execute the command
                    return;
                } else {
                    partial = true;                                 // partial compare so set a flag to indicate this
                }
            }
            
            nextcmd:
                continue;
        }

        if (!partial) {                                              // we have searched the table and have not even found a partial match
            conio_printSimpleCharacter(*vtbuf);                                      // so send the oldest char off
            memcpy(vtbuf, vtbuf + 1, vtcnt--);                      // and shift down the table
        } else {
            return;                                                 // have a partial match so keep the characters in the buffer
        }
    }                                                               // keep looping until the buffer is empty
}

// cursor home
void cmd_CurHome(void) {
    conio_moveCursorHome();
}

// cursor up one or more lines
void cmd_CurUp(void) {
    
    if ((argc == 0) || (arg[0] == 0)) {
        arg[0] = 1;
    }
    
    conio_moveCursorUp(arg[0]);
}

// cursor down one or more lines
void cmd_CurDown(void) {
    
    if ((argc == 0) || (arg[0] == 0)) {
        arg[0] = 1;
    }
    
    conio_moveCursorDown(arg[0]);
}

// cursor left one or more chars
void cmd_CurLeft(void) {
    
    if ((argc == 0) || (arg[0] == 0)) {
        arg[0] = 1;
    }
    
    conio_moveCursorLeft(arg[0]);
}

// cursor right one or more chars
void cmd_CurRight(void) {
    
    if ((argc == 0) || (arg[0] == 0)) {
        arg[0] = 1;
    }
    
    conio_moveCursorRight(arg[0]);
}

// position cursor - note that the argument order is Y, X
void cmd_CurPosition(void) {

    if ((argc < 1) || (arg[0] == 0)) {
        arg[0] = 1;
    }

    if ((argc < 2) || (arg[1] == 0)) {
        arg[1] = 1;
    }

    conio_setNewCursorPosition(arg[1], arg[0]);
}

// clear to end of line
void cmd_ClearEOL(void) {
    /*
    int x, y;
    ShowCursor(false);                                              // turn off the cursor to prevent it from getting confused
    for(y = CursorY; y < CursorY + (fontHeight * fontScale); y++)
        for(x = CursorX; x < HRes; x++)
            plot(x, y, 0);
    */
}

// clear to end of screen
void cmd_ClearEOS(void) {
    /*
    int y, n;
    cmd_ClearEOL();
    y = CursorY + (fontHeight * fontScale);
    n = VBuf*(HBuf/8) - (y * (HBuf/8));
	if(y < VBuf) memset((char *)VideoBuf + (y * (HBuf/8)), 0, n);
    */
}

// clear from the beginning of the line to the cursor
void cmd_ClearBOL(void) {
    /*
    int x, y;
    ShowCursor(false);                                              // turn off the cursor to prevent it from getting confused
    for(y = CursorY; y < CursorY + (fontHeight * fontScale); y++)
        for(x = 0; x < CursorX; x++)
            plot(x, y, 0);
    */
}

// clear from home to the cursor
void cmd_ClearBOS(void) {
    /*
    int x, y;
    cmd_ClearBOL();
    for(y = CursorY - 1; y >= 0; y--)
        for(x = 0; x < HRes; x++)
            plot(x, y, 0);
    */
}

// clear entire screen
void cmd_ClearScreen(void) {
    conio_clearScreenHomeCursor();
}

// turn the cursor off
void cmd_CursorOff(void) {
    conio_hideCursor();
}

// turn the cursor on
void cmd_CursorOn(void) {
    conio_displayCursor();
}

// save the current attributes
void cmd_ClearLine(void) {
    cmd_ClearBOL();
    cmd_ClearEOL();
}

// respond as a VT100 thats OK
void cmd_VT100OK(void) {
    serial_uartSendString("\033[0n");
}

// respond as a VT100 with no optiond
void cmd_VT100ID(void) {
    serial_uartSendString("\033[?1;0c");
}

// reset the terminal
void cmd_Reset(void) {
    system_executeSystemReset();
}

// respond with the current cursor position
void cmd_ReportPosition(void) {

    st_conioCursor *cursorPosition = conio_getCurrentCursorPosition();
    uint8_t msgBuffer[TEXT_COLUMNS_VISIBLE];

    assert (cursorPosition != NULL);

    snprintf(msgBuffer, TEXT_COLUMNS_VISIBLE, "\033[%d;%dR", cursorPosition->currentCursorRow, cursorPosition->currentCursorColumn);
    serial_uartSendString(msgBuffer);
}

// do a line feed
void cmd_LineFeed(void) {
    conio_printSimpleCharacter('\n');
}