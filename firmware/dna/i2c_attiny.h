#ifndef I2C_ATTINY_H
#define I2C_ATTINY_H
/*------------------------------------------------------------------------------*/
/* Copyright: (c) 2013 by Curt Hartung
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#include <avr/io.h>
#include <util/delay.h>

unsigned char i2cStartRead( unsigned char address );
unsigned char i2cStartWrite( unsigned char address );
unsigned char i2cWrite( unsigned char data );
unsigned char i2cReadStream(); // Read a bye, expecting more
unsigned char i2cReadByte(); // Read a single byte and terminate transmission
void i2cInit( unsigned char c );
void i2cStart();
void i2cStop();

#define i2cWait()

//#define i2cSetSCLLow() (DDRA |=  0b00010000); asm volatile("nop"); asm volatile("nop"); asm volatile("nop")//_delay_us(1)
//#define i2cSetSCLHigh() (DDRA &= 0b11101111); asm volatile("nop"); asm volatile("nop"); asm volatile("nop")//_delay_us(1)
//#define i2cSetSDALow() (DDRA |=  0b01000000); asm volatile("nop"); asm volatile("nop"); asm volatile("nop")//_delay_us(1)
//#define i2cSetSDAHigh() (DDRA &= 0b10111111); asm volatile("nop"); asm volatile("nop"); asm volatile("nop")//_delay_us(1)

#define i2cSCL 0b00010000
#define i2cSDA 0b01000000

//extern unsigned char g_i2cDelay;
#define i2cSetSDALow() (DDRA |= i2cSDA); _delay_us(3)//g_i2cDelay)
#define i2cSetSDAHigh() (DDRA &= ~i2cSDA); _delay_us(3)//g_i2cDelay)
#define i2cSetSCLLow() (PORTA &= ~i2cSCL); _delay_us(3)//g_i2cDelay)
#define i2cSetSCLHigh() (PORTA |= i2cSCL); _delay_us(3)//g_i2cDelay)


#define i2cGetSDA() (PINA & i2cSDA)

#endif
