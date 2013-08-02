#ifndef I2C_H
#define I2C_H
/*------------------------------------------------------------------------------*/
/* Copyright: (c) 2013 by Curt Hartung
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#include <avr/io.h>
#include <util/twi.h>

// ((F_CPU/I2C_CLOCK_IN_HZ)-16)/2;

#define i2cInit( c ) \
	TWSR = 0; \
	TWBR = (c);  \
	TWCR = (1<<TWEN); \

#define i2cWait() if ( !(TWCR & (1<<TWINT)) ) { i2cWaitLoop(); }

unsigned char i2cStartRead( unsigned char address );
unsigned char i2cStartWrite( unsigned char address );
#define i2cStart() TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN) i2cWait() 
#define i2cStop() (TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO)) // place a stop condition on the bus

unsigned char i2cWrite( unsigned char data );
unsigned char i2cReadStream(); // Read a bye, expecting more
unsigned char i2cReadByte(); // Read a single byte and terminate transmission

#endif
