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

//------------------------------------------------------------------------------
class Oled
{
public:

	void clear();

	void setPixel( const unsigned char x, const unsigned char y, const bool pixel =true );

	// return the data as it would need to be loaded into the OLED from
	// the microcontroller (upper left to lower right)
	void getPageDump( unsigned char map[512] );

	// get data it 8-bit monochrome bitmap form, data is 128x32
	void getBitmap( unsigned char map[4096] ); 

	Oled() { clear(); }

private:

	unsigned char m_pages[ 4 ][ 128 ];
};

#endif