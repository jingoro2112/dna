#include "24c512.h"
/*------------------------------------------------------------------------------*
 * Copyright: (c) 2013 by Curt Hartung avr@northarc.com
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#include "i2c_interface.h"

//------------------------------------------------------------------------------
void startWrite24c512( unsigned char i2cAddress, int address )
{
	while ( !i2cStartWrite(i2cAddress) );
	i2cWrite( address >> 8 );
	i2cWrite( address );
}

//------------------------------------------------------------------------------
void read24c512( unsigned char i2cAddress, int address, unsigned char* bytes, unsigned char len )
{
	// 'index' the EEPROM by setting up a fake write, this loads the
	// address register with where we want to read, then it's just a
	// matter of shifting out as many bytes as we want (the address
	// auto-increments after each read)
	startWrite24c512( i2cAddress, address );
			
	// now perform a 'normal' i2c read
	while ( !i2cStartRead(i2cAddress) );

	// shift out len - 1 bytes in 'stream' (no ACK sent) where the
	// final bye is as normal, with an ACK, bit of logic to make sure
	// the '1' corner case is properly handled
	unsigned char pos = 0;
	len--;
	for( ; pos != len ; pos++ )
	{
		bytes[pos] = i2cReadStream();
	}

	bytes[pos] = i2cReadByte(); // in all cases the last read is a no-ack style

	i2cStop();
}


//------------------------------------------------------------------------------
void write24c512( unsigned char i2cAddress, int address, unsigned char* bytes, unsigned char len )
{
	// 'index' the eeprom, then stream out as many bytes as were
	// requested, be sure not to exceed 128, that is the size of the intenral write buffer
	startWrite24c512( i2cAddress, address );

	unsigned char pos = 0;
	for( ; pos < len; pos++ )
	{
		i2cWrite( bytes[pos] );
	}
	
	i2cStop();
}

//------------------------------------------------------------------------------
void erase24c512( unsigned char i2cAddress )
{
	for( unsigned int i=0; i<0xFFFF; i++ )
	{
		if ( !(i%128) )
		{
			stop24c512();
			startWrite24c512( i2cAddress, i );
		}

		streamWrite24c512( 0xFF );
	}

	stop24c512();
}

