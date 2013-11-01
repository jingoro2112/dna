/*------------------------------------------------------------------------------
 * Copyright: (c) 2013 by Curt Hartung avr@northarc.com
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#include "menu.h"
#include "eeprom_types.h"
#include "eeprom_image.h"

#include <frame.h>
#include <24c512.h>
#include <text.h>
#include <sram.h>
#include <dstring.h>
#include <cfal12832.h>
#include <avr/eeprom.h>
#include <rna_packet.h>
#include <rna.h>

extern struct OledSettings settings;
void loadEEPROMConstants();
void saveEEPROMConstants();

struct MenuHeader g_currentMenu;

uint16 g_currentMenuEntryOffset;
uint8 g_cursor;
uint8 g_cursorMax;
uint8 g_editing;

int g_repeatCountdown;
uint8 g_lastPress;
uint8 g_repeatCountdownLong;
uint8 g_repeatCountdownShort;

#define SRAM_CONFIG_BLOCK_SIZE 128

//------------------------------------------------------------------------------
enum flags
{
	FlagAddUpDownCarrots = 1<<0,
	FlagDither = 1<<1,
	FlagDisplayValue = 1<<2,
	FlagSunkenValue = 1<<3,
};
   
//------------------------------------------------------------------------------
int computeValueAddress( unsigned int target )
{
	return (SRAM_SIZE - ((target + 1) * SRAM_CONFIG_BLOCK_SIZE));
}

//------------------------------------------------------------------------------
int getValue( struct Entry* entry )
{
	int value = 0;

	uint16 address = computeValueAddress( entry->target ) +  entry->valueOffset;
	if ( entry->type >= Entry16 ) 
	{
		sramRead( address, (unsigned char*)&value, 2 );
	}
	else
	{
		sramRead( address, (unsigned char*)&value, 1 );
	}

	return value;
}

//------------------------------------------------------------------------------
void setValue( struct Entry* entry, int value )
{
	uint16 address = computeValueAddress( entry->target ) +  entry->valueOffset;
	uint8 packet[5];
	packet[0] = RNATypeSetConfigItem;
	packet[1] = entry->valueOffset;
	if ( entry->type >= Entry16 ) 
	{
		sramWrite( address, (unsigned char*)&value, 2 );
		packet[2] = 1;
		packet[3] = value;
	}
	else
	{
		sramWrite( address, (unsigned char*)&value, 1 );
		packet[2] = 0;
		*(int *)(packet + 3) = value;
	}

	rnaSend( entry->target, packet, 5 );
}

//------------------------------------------------------------------------------
unsigned char loadEntry( unsigned char pos, struct Entry *entry )
{
	if ( pos > g_cursorMax )
	{
		return 0;
	}
		
	read24c512( 0xA0,
				g_currentMenuEntryOffset + (pos * sizeof(struct Entry)),
				(unsigned char*)entry,
				sizeof(struct Entry) );

	return 1;
}

//------------------------------------------------------------------------------
void renderEntry( uint8 position, uint8 screenPosX, uint8 screenPosY, uint8 font, uint8 flags )
{
	struct Entry entry;
	if ( !loadEntry(position, &entry) )
	{
		return;
	}
	
	stringAt( EEPROMString(entry.stringItem), screenPosX, screenPosY, font, flags & FlagDither );

	int value = getValue( &entry );
	char buf[MAX_EEPROM_STRLEN + 1];

	if ( entry.valueEnumSet & 0x80 )
	{
		if ( entry.type == Entry10X )
		{
			dsprintf_P( buf, PSTR("%d.%d"), value/10, value%10 );
		}
		else
		{
			dsprintf_P( buf, PSTR("%d"), value );
		}
	}
	else
	{
		unsigned int offset = pgm_read_word( enumVectors + entry.valueEnumSet );

		offset += value * 2;
		
		read24c512( 0xA0, offset, (unsigned char*)&offset, 2 );

//		dsprintf_P( buf, PSTR("%s"), EEPROMString(offset) );

		if ( font != 1 )
		{
			font = 1;
			screenPosY += 2;
		}

		read24c512( 0xA0, offset, (unsigned char *)buf, MAX_EEPROM_STRLEN + 1 );
	}

	int valueWidth = stringFontLen(buf, font);

	if ( (flags & FlagDisplayValue) && (entry.type != EntrySubMenu) )
	{
		int justify = (flags & FlagSunkenValue) ? 125 : 128;
		stringAt( buf, justify - valueWidth, screenPosY, font, flags & FlagDither );
	}

	if ( flags & FlagAddUpDownCarrots )
	{
		int x = 127 - (valueWidth / 2);
		int y = 5;
		if ( value > entry.min )
		{
			frameSetPixel( x, 28 - (y) );
			frameSetPixel( x, 28 - (y+1) );
			frameSetPixel( x - 1, 28 - (y+1) );
			frameSetPixel( x + 1, 28 - (y+1) );
			frameSetPixel( x, 28 - (y+2) );
			frameSetPixel( x - 1, 28 - (y+2) );
			frameSetPixel( x + 1, 28 - (y+2) );
			frameSetPixel( x - 2, 28 - (y+2) );
			frameSetPixel( x + 2, 28 - (y+2) );
		}

		if ( value < entry.max )
		{
			frameSetPixel( x, y );
			frameSetPixel( x, y+1 );
			frameSetPixel( x - 1, y+1 );
			frameSetPixel( x + 1, y+1 );
			frameSetPixel( x, y+2 );
			frameSetPixel( x - 1, y+2 );
			frameSetPixel( x + 1, y+2 );
			frameSetPixel( x - 2, y+2 );
			frameSetPixel( x + 2, y+2 );
		}
	}
}


//------------------------------------------------------------------------------
void menuSetConfigData( unsigned char* data, unsigned char from )
{
	sramWrite( computeValueAddress(from), data, SRAM_CONFIG_BLOCK_SIZE );
}

//------------------------------------------------------------------------------
const PROGMEM unsigned char c_dividerBar[16]=
{ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC, 0xFC, 0x1C, 0x60, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00 };

//------------------------------------------------------------------------------
void menuSetCurrent( uint16 offset )
{
	g_cursor = 0;
	g_editing = 0;

	offset += ROOT_MENU_OFFSET;
	
	read24c512( 0xA0, offset, (unsigned char*)&g_currentMenu, sizeof(struct MenuHeader) );
	g_currentMenuEntryOffset = offset + sizeof( struct MenuHeader );

	for( unsigned char c = 0; c<MENU_ENTRIES; c++ )
	{
		struct Entry entry;
		read24c512( 0xA0, g_currentMenuEntryOffset + (c * sizeof(struct Entry)), (unsigned char*)&entry, sizeof(struct Entry) );
		if ( entry.stringItem )
		{
			g_cursorMax = c;
		}
		else
		{
			break;
		}
	}
}

//------------------------------------------------------------------------------
void menuRender()
{
	uint8 pos = g_cursor;

	// draw selector carrot (dithered)
	frameSetPixel( 0, 12);
	frameSetPixel( 1, 13 );
	frameSetPixel( 2, 14 );
	frameSetPixel( 0, 14 );
	frameSetPixel( 1, 15 );
	frameSetPixel( 0, 16 );

	if ( g_editing )
	{
		// top entry will never be editted, it is always a 'back' button
		renderEntry( pos - 1, 0, 0, 1, FlagDither );
		renderEntry( pos, 5, 8, 2, FlagDisplayValue | FlagAddUpDownCarrots );
		renderEntry( pos + 1, 0, 20, 1, FlagDither );

		// fill in left carrot
		frameSetPixel( 0, 13 );
		frameSetPixel( 1, 14 );
		frameSetPixel( 0, 15 );
	}
	else
	{
		if ( g_cursor == 0 )
		{
			// render title bar
			stringAt( EEPROMString(g_currentMenu.stringTitle), 0, 0, 5, 0 );
			for( unsigned char b=0; b<127; b++ )
			{
				char byte = pgm_read_byte( c_dividerBar + b/8 );
				if ( byte & 1<<b%8 )
				{
					frameSetPixel( b, 8 );
				}
			}

			renderEntry( pos++, 5, 9, 2, FlagDisplayValue );
			renderEntry( pos++, 0, 20, 1, FlagDisplayValue | FlagSunkenValue );
		}
		else
		{
			renderEntry( pos - 1, 0, 0, 1, FlagDisplayValue | FlagSunkenValue );
			renderEntry( pos, 5, 8, 2, FlagDisplayValue );
			renderEntry( pos + 1, 0, 20, 1, FlagDisplayValue | FlagSunkenValue );
		}
	}
}

//------------------------------------------------------------------------------
uint8 processButtonVector( uint8 vector )
{
	struct Entry entry;
	loadEntry( g_cursor, &entry );

	if ( vector & ButtonBitMiddle )
	{
		if ( g_editing )
		{
			g_editing = 0;
			loadEEPROMConstants();
		}
		else if ( vector & ButtonBitDouble )
		{
			menuSetCurrent( 0 ); // return to root
			return 1; // double-click always exits
		}
		else if ( entry.type == EntrySubMenu )
		{
			if ( !entry.valueOffset && (g_currentMenuEntryOffset == (ROOT_MENU_OFFSET + sizeof(struct MenuHeader)) ) )
			{
				return 1; // Exit out of menu
			}
			else
			{
				menuSetCurrent( entry.valueOffset );
			}
		}
		else
		{
			g_editing = 1;
		}
	}
	else if ( vector & ButtonBitTop )
	{
		if ( g_editing )
		{
			int value = getValue( &entry );
			if ( value > entry.min )
			{
				value -= entry.delta;
			}
			else
			{
				value = entry.min;
			}
			setValue( &entry, value );
		}
		else if ( g_cursor < g_cursorMax )
		{
			g_cursor++;
		}
	}
	else if ( vector & ButtonBitBottom )
	{
		if ( g_editing )
		{
			int value = getValue( &entry );
			if ( value < entry.max )
			{
				value += entry.delta;
			}
			else
			{
				value = entry.max;
			}
			setValue( &entry, value );
		}
		else if ( g_cursor > 0 )
		{
			g_cursor--;
		}
	}
	
	return 0;
}

//------------------------------------------------------------------------------
uint8 menuProcessButtonState( uint8 vector )
{
	g_lastPress = vector;
	if ( vector )
	{
		g_repeatCountdown = settings.repeatDelay * 128;
	}
	else
	{
		g_repeatCountdown = 0;
	}

	return processButtonVector( vector );
}

//------------------------------------------------------------------------------
uint8 menuTick( uint8 millisecondDelta )
{
	if ( g_lastPress )
	{
		g_repeatCountdown -= millisecondDelta;
		if ( g_repeatCountdown < 0 )
		{
			g_repeatCountdown = 100;
			processButtonVector( g_lastPress );
			return 1;
		}
	}

	return 0;
}
