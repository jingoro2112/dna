/* Copyright: (c) 2013 by Curt Hartung
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#include "sram.h"

#include <avr/io.h>

//------------------------------------------------------------------------------
void sramStartRead( unsigned int address )
{
	sramClockOutByte( 0b00000011 ); // start a read
	sramClockOutByte( (unsigned char)(address >> 8) );
	sramClockOutByte( (unsigned char)address );
}

//------------------------------------------------------------------------------
void sramStartWrite( unsigned int address )
{
	sramClockOutByte( 0b00000010 ); // start a write
	sramClockOutByte( (unsigned char)(address >> 8) );
	sramClockOutByte( (unsigned char)address );
}

//------------------------------------------------------------------------------
void sramClockOutBit( unsigned char bit )
{
	if( bit )
	{
		sramSetSIHigh();
	}
	else
	{
		sramSetSILow();
	}
	
	sramSetSCKHigh();
	sramSetSCKLow();
}

//------------------------------------------------------------------------------
void sramClockOutByte( unsigned char byte )
{
	unsigned char bit = 0x80;
	do
	{
		if ( byte & bit )
		{
			sramSetSIHigh();
		}
		else
		{
			sramSetSILow();
		}
		sramSetSCKHigh();
		sramSetSCKLow();

	} while( bit >>= 1 );
}

//------------------------------------------------------------------------------
unsigned char sramClockInBit()
{
	sramSetSCKHigh();
	unsigned char ret = sramGetSO();
	sramSetSCKLow();
	return ret;
}

//------------------------------------------------------------------------------
unsigned char sramClockInByte()
{
	unsigned char bit = 0x80;
	unsigned char ret = 0;
	do
	{
		sramSetSCKHigh();
		if ( sramGetSO() )
		{
			ret |= bit;
		}
		sramSetSCKLow();
				
	} while( bit >>= 1 );
	
	return ret;
}

//------------------------------------------------------------------------------
unsigned char sramRead( unsigned int address )
{
	sramSetCSLow();

	sramStartRead( address );
	unsigned char ret = sramClockInByte();
	
	sramSetCSHigh();
	
	return ret;
}

//------------------------------------------------------------------------------
void sramWrite( unsigned int address, unsigned char data )
{
	sramSetCSLow();
	sramStartWrite( address );
	sramClockOutByte( data );
	sramSetCSHigh();
}

//------------------------------------------------------------------------------
void sramInit()
{
	// set up port directions
	SRAM_CS_DDR_L |= (1<<SRAM_CS_PIN);
	SRAM_SI_DDR_L |= (1<<SRAM_SI_PIN);
	SRAM_SO_DDR_L &= ~(1<<SRAM_SO_PIN);
	SRAM_SCK_DDR_L |= (1<<SRAM_SCK_PIN);

	// init state
	sramSetCSHigh();
	sramSetSCKLow();

	// set internal options
	sramSetCSLow();
	sramClockOutByte( 0b00000001 );
	sramClockOutByte( 0b01000001 );
	sramSetCSHigh();
}
