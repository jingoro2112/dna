#ifndef EEPROM_TYPES_H
#define EEPROM_TYPES_H
/*------------------------------------------------------------------------------
 * Copyright: (c) 2013 by Curt Hartung avr@northarc.com
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#include <dna_types.h>

#ifndef __AVR__
#include "../util/str.hpp"
#include <stddef.h>
#pragma pack(push, 1)
#endif

//------------------------------------------------------------------------------
struct FontCharEntry
{
	uint16 dataOffset;
	uint8 w;
	uint8 h;
	uint8 pre;
	uint8 post;
};

#define MENU_ENTRIES 16

//------------------------------------------------------------------------------
enum EntryTypes
{
	Entry8 = 1,
	Entry16,
	EntrySubMenu,
};

//------------------------------------------------------------------------------
struct Entry
{
	uint16 stringItem;
	int8 type;
	int8 target;
	uint16 valueOffset;
	int8 valueEnumSet;
	int16 min;
	int16 max;
};

//------------------------------------------------------------------------------
struct MenuHeader
{
	uint16 stringTitle;
};

//------------------------------------------------------------------------------
struct Menu
{
	struct MenuHeader head;
	struct Entry entry[MENU_ENTRIES];
};

//------------------------------------------------------------------------------
struct OledSettings
{
	uint8 invertDisplay;
	uint8 invertButtons;
	uint8 brightness;
	uint8 repeatDelay;

#ifndef __AVR__
	//------------------------------------------------------------------------------
	static int offsetOf( const char* memberName )
	{
		Cstr compare( memberName );

		if ( compare == "invertDisplay" ) return offsetof( OledSettings, invertDisplay );
		else if ( compare == "brightness" ) return offsetof( OledSettings, brightness );
		else if ( compare == "invertButtons" ) return offsetof( OledSettings, invertButtons );
		else if ( compare == "repeatDelay" ) return offsetof( OledSettings, repeatDelay );
		else return -1;
	}
#endif
};
   
#ifndef __AVR__
#pragma pack(pop)
#endif

#endif
