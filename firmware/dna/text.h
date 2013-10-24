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

#define stringAtResident(string, x, y) stringAtResidentEx(0, string, x, y)
#define stringAtResident_P(string, x, y) stringAtResidentEx(1, string, x, y)
void stringAtResidentEx( char flash, char* string, char x, char y );
   
#define stringAt(string, x, y, font, dither) stringAtEx(0, string, x, y, font, dither, 0)
#define stringAt_P(string, x, y, font, dither) stringAtEx(1, string, x, y, font, dither, 0)
#define stringFontLen( string, font ) stringAtEx(0, string, 0, 0, font, 0, 1)
#define stringFontLen_P( string, font ) stringAtEx(1, string, 0, 0, font, 0, 1)
char stringAtEx( char flash, char* string, char x, char y, unsigned char font, unsigned char dither, unsigned char lenOnly );


#define MAX_EEPROM_STRLEN 32
char* EEPROMString( unsigned int index );

#endif
