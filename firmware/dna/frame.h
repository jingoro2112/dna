#ifndef FRAME_H
#define FRAME_H
/*------------------------------------------------------------------------------*
 * Copyright: (c) 2013 by Curt Hartung avr@northarc.com
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

//------------------------------------------------------------------------------
enum FrameMode
{
	FrameConsole = 0,
	FrameReplay,
	FrameMenu,

	FrameLast,
};

void frameBlit();
void frameSetPixel( char x, char y );
void frameResetPixel( char x, char y );
void frameClear();
void frameLine( int x0, int y0, int x1, int y1 );

#endif
