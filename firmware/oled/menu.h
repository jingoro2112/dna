#ifndef MENU_H
#define MENU_H
/*------------------------------------------------------------------------------
 * Copyright: (c) 2013 by Curt Hartung avr@northarc.com
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#include <dna.h>

void menuSetConfigData( unsigned char* data, unsigned char from );
void menuSetCurrent( uint16 offset );
void menuRender();
void menuLoadSettings();
void menuSetDefaults();
void menuSaveSettings();
uint8 menuProcessButtonState( uint8 vector );
uint8 menuTick( uint8 millisecondDelta ); // should be called every

#endif
