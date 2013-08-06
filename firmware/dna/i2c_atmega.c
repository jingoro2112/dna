#include "i2c.h"
/* Copyright: (c) 2013 by Curt Hartung
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

//------------------------------------------------------------------------------
void i2cWaitLoop()
{
	while(!(TWCR & (1<<TWINT)));	
}

//------------------------------------------------------------------------------
unsigned char i2cStartRead( unsigned char address )
{
	TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN); // send start
	i2cWait();

	TWDR = address | 0x01; // set R/W bit to 'Read'
	TWCR = (1<<TWINT) | (1<<TWEN); // put address on bus
	i2cWait();

	return TW_STATUS == TW_MR_SLA_ACK;
}

//------------------------------------------------------------------------------
unsigned char i2cStartWrite( unsigned char address )
{
	TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN); // send start
	i2cWait();
	
	TWDR = address & 0xFE; // set R/W bit to 'Write'
	TWCR = (1<<TWINT) | (1<<TWEN); // put address on bus

	i2cWait();

	return TW_STATUS == TW_MT_SLA_ACK; // was there someone out there?
}

//------------------------------------------------------------------------------
unsigned char i2cWrite( unsigned char data )
{	
	i2cWait();
    
	// send data to the previously addressed device
	TWDR = data;
	TWCR = (1<<TWINT) | (1<<TWEN);

	i2cWait();

	return TW_STATUS == TW_MT_SLA_ACK;
}

//------------------------------------------------------------------------------
void i2cWriteNoWait( unsigned char data )
{	
	i2cWait();

	// send data to the previously addressed device
	TWDR = data;
	TWCR = (1<<TWINT) | (1<<TWEN);
}

//------------------------------------------------------------------------------
// Read a bye, expecting more
unsigned char i2cReadStream()
{
	i2cWait();
	TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWEA);
	i2cWait();
    return TWDR;
}

//------------------------------------------------------------------------------
// Read a single byte and terminate transmission
unsigned char i2cReadByte()
{
	i2cWait();
	TWCR = (1<<TWINT) | (1<<TWEN);
	i2cWait();
    return TWDR;
}
