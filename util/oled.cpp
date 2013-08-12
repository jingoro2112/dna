#include "oled.hpp"
/* Copyright: (c) 2013 by Curt Hartung
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */


// algorithm testing for the OLED

//------------------------------------------------------------------------------
void Oled::clear()
{
	for( int i=0; i<4; i++ )
	{
		for( int j=0; j<128; j++ )
		{
			m_pages[i][j] = 0;
		}
	}
}

//------------------------------------------------------------------------------
void Oled::setPixel( const unsigned char x, const unsigned char y, const bool pixel /*=true*/ )
{
	if ( x >= 128 || y >= 32 )
	{
		return;
	}
		
	int page = y >> 3;
	if ( pixel )
	{
		m_pages[ y>>3 ][ x ] |= (1 << (y & 0x07));
	}
	else
	{
		m_pages[ y>>3 ][ x ] &= ~(1 << (y & 0x07));
	}
}


//------------------------------------------------------------------------------
void Oled::getPageDump( unsigned char map[512] )
{
	int index = 0;
	for( int i=0; i<4; i++ )
	{
		for( int j=0; j<128; j++ )
		{
			map[index++] = m_pages[i][j];
		}
	}
}

//------------------------------------------------------------------------------
void Oled::getBitmap( unsigned char map[4096] )
{
	int index = 0;
	for( int y=0; y<32; y++ )
	{
		for( int x=0; x<128; x++ )
		{
			map[index++] = (m_pages[y>>3][x] & (1 << (y & 0x07))) ? 0xFF : 0;
		}
	}
}
