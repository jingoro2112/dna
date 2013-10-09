#ifndef OLED_HPP
#define OLED_HPP
/*------------------------------------------------------------------------------*/
/* Copyright: (c) 2013 by Curt Hartung
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

// This class exists only as a testbed for the Morlock OLED module from
// CrystalFontz: http://www.crystalfontz.com/product/CFAL12832DB

#include <memory.h>

//------------------------------------------------------------------------------
class Oled
{
public:

	void characterAt( char c, unsigned char x, unsigned char y, unsigned char font );
	void stringAt( char *string, unsigned char x, unsigned char y, unsigned char font );
	
	void clear() { memset( m_screen, 0, 512); }
	int computeAddress( unsigned char x, unsigned char y ) { return ((y & 0xF8) << 4) + x; }
	void setPixel( const unsigned char x, const unsigned char y, const bool pixel =true );
	void setRaster( const unsigned char column, const unsigned char row, const unsigned char raster ) { m_screen[(int)row*128 + (int)column] = raster; }

	// return the data as it would need to be loaded into the OLED from
	// the microcontroller (upper left to lower right)
	const unsigned char* getScreen() const { return m_screen; }

	// get data it 8-bit monochrome bitmap form, data is 128x32

	Oled() { clear(); }

private:

	unsigned char m_screen[ 512 ];
};

#endif