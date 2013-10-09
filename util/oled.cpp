#include "oled.hpp"
/*------------------------------------------------------------------------------*
 * Copyright: (c) 2013 by Curt Hartung avr@northarc.com
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#include "..\firmware\dna\dna_types.h"
#include "..\oled\font_loader.h"

//------------------------------------------------------------------------------
void Oled::characterAt( char c, unsigned char x, unsigned char y, unsigned char font )
{
	unsigned int offset = ((c - 32) * (sizeof(FontCharEntry) * NUMBER_OF_FONTS)) + (sizeof(FontCharEntry) * font);
	
	FontCharEntry* entry = (FontCharEntry *)(c_lookupTable + offset);

	const unsigned char *byte = c_dataBlock + entry->dataOffset;
	
	for( unsigned char h=0; h<entry->h; h++ )
	{
		unsigned char bit = 0;
		for( unsigned char w = 0; w<entry->w; w++ )
		{
			if ( 1<<bit & *byte )
			{
				setPixel( x + w, y + h, true );
			}
			
			if ( ++bit == 8 )
			{
				byte++;
				bit = 0;
			}
		}

		if ( bit )
		{
			byte++;
		}
	}
}

//------------------------------------------------------------------------------
void Oled::stringAt( char *string, unsigned char x, unsigned char y, unsigned char font )
{
	for( ;*string ;string++ )
	{
		unsigned int offset = ((*string - 32) * (sizeof(FontCharEntry) * NUMBER_OF_FONTS)) + (sizeof(FontCharEntry) * font);
		FontCharEntry* entry = (FontCharEntry *)(c_lookupTable + offset);

		x += entry->pre;
		
		const unsigned char *byte = c_dataBlock + entry->dataOffset;

		for( unsigned char h=0; h<entry->h; h++ )
		{
			unsigned char bit = 0;
			for( unsigned char w = 0; w<entry->w; w++ )
			{
				if ( 1<<bit & *byte )
				{
					setPixel( x + w, y + h, true );
				}

				if ( ++bit == 8 )
				{
					byte++;
					bit = 0;
				}
			}

			if ( bit )
			{
				byte++;
			}
		}

		x += entry->post + entry->w;
	}
}

//------------------------------------------------------------------------------
void Oled::setPixel( const unsigned char x, const unsigned char y, const bool pixel /*=true*/ )
{
	if ( x < 128 && y < 32 )
	{
		if ( pixel )
		{
			m_screen[computeAddress( x, y )] |= (1 << (y & 0x07));
		}
		else
		{
			m_screen[computeAddress( x, y )] &= ~(1 << (y & 0x07));
		}
	}
}

