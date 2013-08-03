/* Copyright: (c) 2013 by Curt Hartung
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#include "sram.h"

#include <avr/io.h>

//------------------------------------------------------------------------------
void sramClock( unsigned char c )
{
	if ( c == 0 )
	{
		sramSetSILow();
	}
	else
	{
		sramSetSIHigh();
	}
	sramSetSCKHigh();
	sramSetSCKLow();
}

//------------------------------------------------------------------------------
void sramClockAddress( unsigned int address )
{
	sramClock( 0 );
	unsigned char a1 = address >> 8;
	sramClock( a1 & 0b01000000 );
	sramClock( a1 & 0b00100000 );
	sramClock( a1 & 0b00010000 );
	sramClock( a1 & 0b00001000 );
	sramClock( a1 & 0b00000100 );
	sramClock( a1 & 0b00000010 );
	sramClock( a1 & 0b00000001 );
	sramClock( address & 0b10000000 );
	sramClock( address & 0b01000000 );
	sramClock( address & 0b00100000 );
	sramClock( address & 0b00010000 );
	sramClock( address & 0b00001000 );
	sramClock( address & 0b00000100 );
	sramClock( address & 0b00000010 );
	sramClock( address & 0b00000001 );
}

//------------------------------------------------------------------------------
unsigned char sramRead( unsigned int address )
{
	sramSetCSLow();

	sramClock( 0 );
	sramClock( 0 );
	sramClock( 0 );
	sramClock( 0 );
	sramClock( 0 );
	sramClock( 0 );
	sramClock( 1 );
	sramClock( 1 );

	sramClockAddress( address );

	unsigned char ret = 0;
	sramSetSCKHigh();
	if ( sramGetSO() )
	{
		ret += 0b10000000;
	}
	sramSetSCKLow();
	sramSetSCKHigh();
	if ( sramGetSO() )
	{
		ret += 0b01000000;
	}
	sramSetSCKLow();
	sramSetSCKHigh();
	if ( sramGetSO() )
	{
		ret += 0b00100000;
	}
	sramSetSCKLow();
	sramSetSCKHigh();
	if ( sramGetSO() )
	{
		ret += 0b00010000;
	}
	sramSetSCKLow();
	sramSetSCKHigh();
	if ( sramGetSO() )
	{
		ret += 0b00001000;
	}
	sramSetSCKLow();
	sramSetSCKHigh();
	if ( sramGetSO() )
	{
		ret += 0b00000100;
	}
	sramSetSCKLow();
	sramSetSCKHigh();
	if ( sramGetSO() )
	{
		ret += 0b00000010;
	}
	sramSetSCKLow();
	sramSetSCKHigh();
	if ( sramGetSO() )
	{
		ret += 0b00000001;
	}
	sramSetSCKLow();

	sramSetCSHigh();
	return ret;
}

//------------------------------------------------------------------------------
void sramWrite( unsigned int address, unsigned char data )
{
	sramSetCSLow();

	sramClock( 0 );
	sramClock( 0 );
	sramClock( 0 );
	sramClock( 0 );
	sramClock( 0 );
	sramClock( 0 );
	sramClock( 1 );
	sramClock( 0 );

	sramClockAddress( address );

	sramClock( data & 0b10000000 );
	sramClock( data & 0b01000000 );
	sramClock( data & 0b00100000 );
	sramClock( data & 0b00010000 );
	sramClock( data & 0b00001000 );
	sramClock( data & 0b00000100 );
	sramClock( data & 0b00000010 );
	sramClock( data & 0b00000001 );

	sramSetCSHigh();
}


//------------------------------------------------------------------------------
void sramInit()
{
	sramSetCSHigh();
	sramSetSCKLow();
}
