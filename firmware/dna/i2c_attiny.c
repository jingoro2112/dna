/* Copyright: (c) 2013 by Curt Hartung
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#include "i2c_attiny.h"

//unsigned char g_i2cDelay;

//------------------------------------------------------------------------------
unsigned char i2cWrite( unsigned char data )
{
	if ( data & 0b10000000 )
	{
		i2cSetSDAHigh();
	}
	else
	{
		i2cSetSDALow();
	}
	i2cSetSCLHigh();
	i2cSetSCLLow();

	if ( data & 0b01000000 )
	{
		i2cSetSDAHigh();
	}
	else
	{
		i2cSetSDALow();
	}
	i2cSetSCLHigh();
	i2cSetSCLLow();

	if ( data & 0b00100000 )
	{
		i2cSetSDAHigh();
	}
	else
	{
		i2cSetSDALow();
	}
	i2cSetSCLHigh();
	i2cSetSCLLow();

	if ( data & 0b00010000 )
	{
		i2cSetSDAHigh();
	}
	else
	{
		i2cSetSDALow();
	}
	i2cSetSCLHigh();
	i2cSetSCLLow();

	if ( data & 0b00001000 )
	{
		i2cSetSDAHigh();
	}
	else
	{
		i2cSetSDALow();
	}
	i2cSetSCLHigh();
	i2cSetSCLLow();

	if ( data & 0b00000100 )
	{
		i2cSetSDAHigh();
	}
	else
	{
		i2cSetSDALow();
	}
	i2cSetSCLHigh();
	i2cSetSCLLow();

	if ( data & 0b00000010 )
	{
		i2cSetSDAHigh();
	}
	else
	{
		i2cSetSDALow();
	}
	i2cSetSCLHigh();
	i2cSetSCLLow();

	if ( data & 0b00000001 )
	{
		i2cSetSDAHigh();
	}
	else
	{
		i2cSetSDALow();
	}
	i2cSetSCLHigh();
	i2cSetSCLLow();


	// check for ACK
	i2cSetSDAHigh();
	i2cSetSCLHigh();
	unsigned char ret = 0;
	if ( i2cGetSDA() )
	{
		ret = 1;
	}
	i2cSetSCLLow();
	return ret;
}

//------------------------------------------------------------------------------
unsigned char i2cStartRead( unsigned char address )
{
	i2cStart();
	return i2cWrite( address | 0x01 );
}

//------------------------------------------------------------------------------
unsigned char i2cStartWrite( unsigned char address )
{
	i2cStart();
	return i2cWrite( address & 0xFE );
}

//------------------------------------------------------------------------------
// Read a bye, expecting more
unsigned char i2cReadStream()
{
	unsigned char ret = 0;
	i2cSetSCLHigh();
	if ( i2cGetSDA() )
	{
		ret |= 0b10000000;
	}
	i2cSetSCLLow();
	i2cSetSCLHigh();
	if ( i2cGetSDA() )
	{
		ret |= 0b01000000;
	}
	i2cSetSCLLow();
	i2cSetSCLHigh();
	if ( i2cGetSDA() )
	{
		ret |= 0b00100000;
	}
	i2cSetSCLLow();
	i2cSetSCLHigh();
	if ( i2cGetSDA() )
	{
		ret |= 0b00010000;
	}
	i2cSetSCLLow();
	i2cSetSCLHigh();
	if ( i2cGetSDA() )
	{
		ret |= 0b00001000;
	}
	i2cSetSCLLow();
	i2cSetSCLHigh();
	if ( i2cGetSDA() )
	{
		ret |= 0b00000100;
	}
	i2cSetSCLLow();
	i2cSetSCLHigh();
	if ( i2cGetSDA() )
	{
		ret |= 0b00000010;
	}
	i2cSetSCLLow();
	i2cSetSCLHigh();
	if ( i2cGetSDA() )
	{
		ret |= 0b00000001;
	}
	i2cSetSCLLow();

	i2cSetSDALow(); // ack
	i2cSetSCLHigh();
	i2cSetSCLLow();
	i2cSetSDAHigh();
	
	return ret;
}

//------------------------------------------------------------------------------
// Read a single byte and terminate transmission
unsigned char i2cReadByte()
{
	unsigned char ret = 0;
	i2cSetSCLHigh();
	if ( i2cGetSDA() )
	{
		ret |= 0b10000000;
	}
	i2cSetSCLLow();
	i2cSetSCLHigh();
	if ( i2cGetSDA() )
	{
		ret |= 0b01000000;
	}
	i2cSetSCLLow();
	i2cSetSCLHigh();
	if ( i2cGetSDA() )
	{
		ret |= 0b00100000;
	}
	i2cSetSCLLow();
	i2cSetSCLHigh();
	if ( i2cGetSDA() )
	{
		ret |= 0b00010000;
	}
	i2cSetSCLLow();
	i2cSetSCLHigh();
	if ( i2cGetSDA() )
	{
		ret |= 0b00001000;
	}
	i2cSetSCLLow();
	i2cSetSCLHigh();
	if ( i2cGetSDA() )
	{
		ret |= 0b00000100;
	}
	i2cSetSCLLow();
	i2cSetSCLHigh();
	if ( i2cGetSDA() )
	{
		ret |= 0b00000010;
	}
	i2cSetSCLLow();
	i2cSetSCLHigh();
	if ( i2cGetSDA() )
	{
		ret |= 0b00000001;
	}
	i2cSetSCLLow();

	i2cSetSDALow(); // ack
	i2cSetSCLHigh();
	i2cSetSCLLow();
	i2cSetSDAHigh();

	return ret;
}

//------------------------------------------------------------------------------
void i2cInit( unsigned char c )
{
//	g_i2cDelay = c;
	DDRA  |= i2cSCL | i2cSDA; // set as output
	PORTA &= ~(i2cSCL | i2cSDA); // assert low
	DDRA  &= ~(i2cSCL | i2cSDA); // set as input
	PORTA |= (i2cSCL | i2cSDA); // enable pullups
}

//------------------------------------------------------------------------------
void i2cStart()
{
	i2cSetSDALow(); // SDA low while SCL is high
	i2cSetSCLLow(); // prepare to start clocking
}

//------------------------------------------------------------------------------
void i2cStop()
{
	i2cSetSCLHigh();
	i2cSetSDAHigh(); // SDA high while SCL is high
}
