/*------------------------------------------------------------------------------*
 * Copyright: (c) 2013 by Curt Hartung avr@northarc.com
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#include "sram.h"

#include <avr/io.h>

//------------------------------------------------------------------------------
void sramStartRead( unsigned int address )
{
	sramSetCSLow();
	sramWriteByte( 0b00000011 ); // start a read
	sramWriteByte( (unsigned char)(address >> 8) );
	sramWriteByte( (unsigned char)address );
}

//------------------------------------------------------------------------------
void sramStartWrite( unsigned int address )
{
	sramSetCSLow();
	sramWriteByte( 0b00000010 ); // start a write
	sramWriteByte( (unsigned char)(address >> 8) );
	sramWriteByte( (unsigned char)address );
}

//------------------------------------------------------------------------------
void sramWriteByte( unsigned char byte )
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
		sramDelay();
		sramSetSCKHigh();
		sramDelay();
		sramSetSCKLow();

	} while( bit >>= 1 );
}

//------------------------------------------------------------------------------
unsigned char sramReadByte()
{
	unsigned char bit = 0x80;
	unsigned char ret = 0;
	do
	{
		sramDelay();
		sramSetSCKHigh();
		if ( sramGetSO() )
		{
			ret |= bit;
		}
		sramDelay();
		sramSetSCKLow();
				
	} while( bit >>= 1 );
	
	return ret;
}

//------------------------------------------------------------------------------
unsigned char sramAtomicRead( unsigned int address )
{
	sramStartRead( address );
	unsigned char ret = sramReadByte();
	sramStop();
	return ret;
}

//------------------------------------------------------------------------------
void sramAtomicWrite( unsigned int address, unsigned char data )
{
	sramStartWrite( address );
	sramWriteByte( data );
	sramStop();
}

//------------------------------------------------------------------------------
void sramInit()
{
	// set up port directions
	SRAM_CS_DDR_L |= (1<<SRAM_CS_PIN); // chip select
	SRAM_SI_DDR_L |= (1<<SRAM_SI_PIN); // serial in (output to the SRAM)
	SRAM_SO_DDR_L &= ~(1<<SRAM_SO_PIN); // serial out (input from the SRAM)
	SRAM_SO_PORT_L |= (1<<SRAM_SO_PIN); // turn on pullup
	SRAM_SCK_DDR_L |= (1<<SRAM_SCK_PIN); // clock

	// init state
	sramSetCSHigh();
	sramSetSCKLow();

	// set internal options
	sramSetCSLow();
	sramWriteByte( 0b00000001 ); // write to status register
	sramWriteByte( 0b01000001 ); // set sequential mode
	sramSetCSHigh();
}
