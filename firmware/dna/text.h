#ifndef TEXT_H
#define TEXT_H
/*------------------------------------------------------------------------------
 * Copyright: (c) 2013 by Curt Hartung avr@northarc.com
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

void setPixel( char x, char y ); // must be user-defined
extern unsigned char numberOfFonts;
extern unsigned int dataBlockOrigin;

//#define RESIDENT_FONT_TABLES

#define stringAtResident(string, x, y) stringAtResidentEx(0, string, x, y)
#define stringAtResident_P(string, x, y) stringAtResidentEx(1, string, x, y)
void stringAtResidentEx( char flash, char* string, char x, char y );
   
#ifdef RESIDENT_FONT_TABLES
#define stringAt(string, x, y, font, dither) stringAtEmbeddedEx(0, string, x, y, font, dither)
#define stringAt_P(string, x, y, font, dither) stringAtEmbeddedEx(1, string, x, y, font, dither)
void stringAtEmbeddedEx( char flash, char* string, char x, char y, unsigned char font, unsigned char dither );
#else
#define stringAt(string, x, y, font, dither) stringAtEx(0, string, x, y, font, dither)
#define stringAt_P(string, x, y, font, dither) stringAtEx(1, string, x, y, font, dither)
void stringAtEx( char flash, char* string, char x, char y, unsigned char font, unsigned char dither );
#endif

#define MAX_EEPROM_STRLEN 32
char* EEPROMString( unsigned int index );

#endif
