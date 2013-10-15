#ifndef EEPROM_TYPES_H
#define EEPROM_TYPES_H
/*------------------------------------------------------------------------------
 * Copyright: (c) 2013 by Curt Hartung avr@northarc.com
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#include "../firmware/dna/dna_types.h"

#ifdef _WIN32
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
	uint16 valueOffset;
	int16 min;
	int16 max;
};

//------------------------------------------------------------------------------
struct MenuHeader
{
	uint16 stringTitle;
	uint16 parent;
};

//------------------------------------------------------------------------------
struct Menu
{
	struct MenuHeader head;
	struct Entry entry[MENU_ENTRIES];
};

#ifdef _WIN32
#pragma pack(pop)
#endif

#endif
