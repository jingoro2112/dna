/*------------------------------------------------------------------------------*
 * Copyright: (c) 2013 by Curt Hartung avr@northarc.com
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#include "i2c_brute.h"
#include "i2c_interface.h"

#include <util/delay.h>
#include <avr/io.h>

//------------------------------------------------------------------------------
void i2cInit()
{
	i2cBruteSDAHigh();
	i2cBruteSCKHigh();
	I2C_BRUTE_SDA_PORT &= ~(1<<I2C_BRUTE_SDA_PIN_NUMBER);
	I2C_BRUTE_SCK_PORT &= ~(1<<I2C_BRUTE_SCK_PIN_NUMBER);
}

//------------------------------------------------------------------------------
void i2cWait()
{

}

//------------------------------------------------------------------------------
void i2cStart()
{
	i2cBruteSCKHigh();
	_delay_us( I2C_BRUTE_DELAY_US/2 );
	i2cBruteSDALow(); // SDA low while SCL is high
	_delay_us( I2C_BRUTE_DELAY_US );
	i2cBruteSCKLow(); // prepare to start clocking
	_delay_us( I2C_BRUTE_DELAY_US/2 );
}

//------------------------------------------------------------------------------
void i2cStop()
{
	i2cBruteSDALow(); // SDA high while SCK is high
	_delay_us( I2C_BRUTE_DELAY_US/2 );
	i2cBruteSCKHigh(); 
	_delay_us( I2C_BRUTE_DELAY_US );
	i2cBruteSDAHigh(); // SDA high while SCK is high
	_delay_us( I2C_BRUTE_DELAY_US/2 );
}

//------------------------------------------------------------------------------
unsigned char i2cWrite( unsigned char data )
{
	unsigned char bit = 0x80;
	do
	{
		if ( data & bit )
		{
			i2cBruteSDAHigh();
		}
		else
		{
			i2cBruteSDALow();
		}

		_delay_us( I2C_BRUTE_DELAY_US/2 );
		i2cBruteSCKHigh();
		while( i2cBruteIsSCKLow() );
		_delay_us( I2C_BRUTE_DELAY_US );
		i2cBruteSCKLow();
		_delay_us( I2C_BRUTE_DELAY_US/2 );
		
	} while( bit >>= 1 );
	
	i2cBruteSDAHigh();
	_delay_us( I2C_BRUTE_DELAY_US/2 );
	i2cBruteSCKHigh();
	while( i2cBruteIsSCKLow() );
	_delay_us( I2C_BRUTE_DELAY_US );
	unsigned char ret = 0;
	if ( i2cBruteIsSDALow() )
	{
		ret = 1;
	}
	i2cBruteSCKLow();
	_delay_us( I2C_BRUTE_DELAY_US/2 );
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
		i2cBruteSCKHigh();
		_delay_us( I2C_BRUTE_DELAY_US );
		if ( i2cBruteIsSDAHigh() )
		{
			ret |= bit;
		}
		i2cBruteSCKLow();
		_delay_us( I2C_BRUTE_DELAY_US );
		 
	} while( bit >>= 1 );
		
	i2cBruteSDALow(); // ack
	_delay_us( I2C_BRUTE_DELAY_US/2 );
	i2cBruteSCKHigh();
	_delay_us( I2C_BRUTE_DELAY_US );
	i2cBruteSCKLow();
	_delay_us( I2C_BRUTE_DELAY_US/2 );
	i2cBruteSDAHigh();
	_delay_us( I2C_BRUTE_DELAY_US/2 );
	
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
		i2cBruteSCKHigh();
		_delay_us( I2C_BRUTE_DELAY_US );
		if ( i2cBruteIsSDAHigh() )
		{
			ret |= bit;
		}
		i2cBruteSCKLow();
		_delay_us( I2C_BRUTE_DELAY_US );

	} while( bit >>= 1 );

	return ret;
}

