#ifndef BUTTON_H 
#define BUTTON_H
/*------------------------------------------------------------------------------*
 * Copyright: (c) 2013 by Curt Hartung avr@northarc.com
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

//------------------------------------------------------------------------------
enum ButtonRequest
{
	ButtonRequestStatus = 1,
	ButtonRequestPowerOff,
};

//------------------------------------------------------------------------------
enum ButtonBit
{
	button1 = 1<<0,
	button2 = 1<<1,
	button3 = 1<<2,
};

#endif