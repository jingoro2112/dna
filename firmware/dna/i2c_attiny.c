/*------------------------------------------------------------------------------*
 * Copyright: (c) 2013 by Curt Hartung avr@northarc.com
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#include "i2c_attiny.h"

//------------------------------------------------------------------------------
unsigned char i2cWrite( unsigned char data )
{
	unsigned char bit = 0x80;
	do
	{
		if ( data & bit )
		{
			i2cSetSDAHigh();
		}
		else
		{
			i2cSetSDALow();
		}
		_delay_us( I2C_DELAY_CONST );
		i2cSetSCLHigh();
		_delay_us( I2C_DELAY_CONST );
		i2cSetSCLLow();
		_delay_us( I2C_DELAY_CONST );
		
	} while( bit >>= 1 );
	
	_delay_us( I2C_DELAY_CONST );
	i2cSetSDAHigh();
	i2cSetSCLHigh();
	_delay_us( I2C_DELAY_CONST );
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
	unsigned char bit = 0x80;
	unsigned char ret = 0;
	do
	{
		i2cSetSCLHigh();
		_delay_us( I2C_DELAY_CONST );
		if ( i2cGetSDA() )
		{
			ret |= bit;
		}
		i2cSetSCLLow();
		_delay_us( I2C_DELAY_CONST );
		 
	} while( bit >>= 1 );
		
	i2cSetSDALow(); // ack
	_delay_us( I2C_DELAY_CONST );
	i2cSetSCLHigh();
	_delay_us( I2C_DELAY_CONST );
	i2cSetSCLLow();
	_delay_us( I2C_DELAY_CONST );
	i2cSetSDAHigh();
	_delay_us( I2C_DELAY_CONST );
	
	return ret;
}

//------------------------------------------------------------------------------
// Read a single byte and terminate transmission
unsigned char i2cReadByte()
{
	unsigned char bit = 0x80;
	unsigned char ret = 0;
	do
	{
		i2cSetSCLHigh();
		_delay_us( I2C_DELAY_CONST );
		if ( i2cGetSDA() )
		{
			ret |= bit;
		}
		i2cSetSCLLow();
		_delay_us( I2C_DELAY_CONST );

	} while( bit >>= 1 );

	return ret;
}

//------------------------------------------------------------------------------
void i2cInit()
{
	PORTA |= i2cSCL | i2cSDA;
	DDRA |= i2cSCL | i2cSDA;
}

//------------------------------------------------------------------------------
void i2cStart()
{
	i2cSetSDAHigh();
	i2cSetSDALow(); // SDA low while SCL is high
	_delay_us( I2C_DELAY_CONST );
	i2cSetSCLLow(); // prepare to start clocking
	_delay_us( I2C_DELAY_CONST );
}

//------------------------------------------------------------------------------
void i2cStop()
{
	i2cSetSDALow();
	i2cSetSCLHigh();
	_delay_us( I2C_DELAY_CONST );
	i2cSetSDAHigh(); // SDA high while SCL is high
	_delay_us( I2C_DELAY_CONST );
}
