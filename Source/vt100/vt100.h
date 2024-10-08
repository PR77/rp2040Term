/****************************************************************************************
 vt100.h

	Header file for the vt100 decoder for the VT100 Terminal program


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

#ifndef VT100_H
#define	VT100_H

typedef struct {
    uint8_t *   name;                       // the string
    uint8_t     mode;                       // 1 = ANSI only, 2 = VT52, 3 = both
    void        (* commandHandler)(void);   // pointer to the function that will interpret that command
} st_vt100CommandTable;

void vt100_putCharacter(char character);

void cmd_CurHome(void);
void cmd_CurUp(void);
void cmd_CurDown(void);
void cmd_CurLeft(void);
void cmd_CurRight(void);
void cmd_CurPosition(void);
void cmd_ClearEOL(void);
void cmd_ClearEOS(void);
void cmd_ClearBOL(void);
void cmd_ClearBOS(void);
void cmd_ClearScreen(void);
void cmd_CursorOff(void);
void cmd_CursorOn(void);
void cmd_ClearLine(void);
void cmd_VT100OK(void);
void cmd_VT100ID(void);
void cmd_Reset(void);
void cmd_ReportPosition(void);
void cmd_LineFeed(void);

extern int AttribUL, AttribRV, AttribInvis;                         // attributes that can be turned on/off

#define VT100   1
#define VT52    2
#define BOTH    3

#endif // VT100_H
