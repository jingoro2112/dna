/*------------------------------------------------------------------------------*
 * Copyright: (c) 2013 by Curt Hartung avr@northarc.com
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#include "frame.h"

#include <i2c_brute.h>
#include <24c512.h>
#include <sram.h>
#include <cfal12832.h>
#include <dna.h>
#include <stdlib.h>

//------------------------------------------------------------------------------
void frameBlit()
{
	i2cStartWrite( OLED_ADDRESS );

	i2cWrite( 0x80 );
	i2cWrite( 0x00 );

	i2cWrite( 0x80 );
	i2cWrite( 0x10 );

	i2cWrite( 0x80 );
	i2cWrite( 0xB0 );

	sramStartRead( 0 ); // 0th location is the frame buffer

	for( unsigned int i=0; i<0x200; i++ )
	{
		i2cWrite( 0xC0 );
		i2cWrite( sramReadByte() );
	}

	sramStop();
	i2cWait();
	i2cStop();

}

//------------------------------------------------------------------------------
static uint16 computeAddress( unsigned char x, unsigned char y )
{
	return ((y & 0xF8) << 4) + x;
}

//------------------------------------------------------------------------------
void frameSetPixel( char x, char y )
{
	if ( (unsigned char)x < 128 && (unsigned char)y < 32 )
	{
		uint16 address = computeAddress( x, y );
		sramAtomicWrite( address, sramAtomicRead(address) | (1 << (y & 0x07)) );
	}
}

//------------------------------------------------------------------------------
void frameResetPixel( char x, char y )
{
	if ( (unsigned char)x < 128 && (unsigned char)y < 32 )
	{
		uint16 address = computeAddress( x, y );
		sramAtomicWrite( address, sramAtomicRead(address) & ~(1 << (y & 0x07)) );
	}
}

//------------------------------------------------------------------------------
void frameClear()
{
	sramStartWrite( 0 );
	for( unsigned int i=0; i<0x200; i++ )
	{
		sramWriteByte( 0 );
	}
	sramStop();
}

//------------------------------------------------------------------------------
void frameLine( int x0, int y0, int x1, int y1 )
{
	int dx;
	int sx;
	if ( x0 < x1 )
	{
		sx = 1;
		dx = x1 - x0;
	}
	else
	{
		sx = -1;
		dx = x0 - x1;
	}

	int dy;
	int sy;
	if ( y0 < y1 )
	{
		dy = y1 - y0;
		sy = 1;
	}
	else
	{
		dy = y0 - y1;
		sy = -1;
	}

	int err = dx > dy ? dx : -dy;
	int e2;
	err /= 2;

	for(;;)
	{
		frameSetPixel( x0, y0 );
		
		if ( x0==x1 && y0==y1 )
		{
			break;
		}

		e2 = err;

		if (e2 >-dx)
		{
			err -= dy;
			x0 += sx;
		}

		if (e2 < dy)
		{
			err += dx;
			y0 += sy;
		}
	}
}
