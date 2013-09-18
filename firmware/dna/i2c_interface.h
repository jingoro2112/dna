#ifndef I2C_INTERFACE_H
#define I2C_INTERFACE_H
/* Copyright: (c) 2013 by Curt Hartung avr@northarc.com
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

// must be defined externally
void i2cInit();
void i2cWait();
void i2cStart();
void i2cStop();
unsigned char i2cStartRead( unsigned char address );
unsigned char i2cStartWrite( unsigned char address );
unsigned char i2cWrite( unsigned char data );
unsigned char i2cReadStream();
unsigned char i2cReadByte();

#endif
