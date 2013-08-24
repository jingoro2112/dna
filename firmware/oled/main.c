/* Copyright: (c) 2013 by Curt Hartung
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#include <dna.h>
//#include <usb.h>
#include <oled.h>
#include <i2c.h>
#include <24c512.h>
#include <rna.h>

#include "sram.h"

#include <util/delay.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#define ledOn()  (PORTB &= 0b11111110)
#define ledOff() (PORTB |= 0b00000001)

#define On()  (PORTD |= 0b01000000)
#define Off() (PORTD &= 0b10111111)

unsigned char message[10];
//------------------------------------------------------------------------------
unsigned char dnaUsbInputSetup( unsigned char *data, unsigned char len )
{
//	dnaUsbQueueData( message, 4 );
	return 0;
}

//------------------------------------------------------------------------------
void dnaUsbInputStream( unsigned char *data, unsigned char len )
{
	
}

static unsigned char check;
//------------------------------------------------------------------------------
unsigned char rnaInputSetup( unsigned char *data, unsigned char from, unsigned char len )
{
//	if ( data[0] == check + 1 )
	{
//		check += 2;
		message[0] = data[0];// + 1;
//		rnaSend( from, message, 1 );
	}
	return 1;
}

//------------------------------------------------------------------------------
void rnaInputStream( unsigned char *data, unsigned char bytes )
{

}

//------------------------------------------------------------------------------
void sram2oled( unsigned int address )
{
	i2cStartWrite( OLED_ADDRESS );
	i2cWrite( 0x80 ); // command setup
	i2cWrite( 0x00 );
	i2cWrite( 0x80 ); // command setup
	i2cWrite( 0x10 );
	i2cWrite( 0x80 ); // command setup
	i2cWrite( 0xB0 );

	sramStartRead( address );

	unsigned int i;
	unsigned char ret;
	for( i=0; i<512; i++ )
	{
		ret = sramClockOutByte();
		i2cWrite( 0xC0 ); // data setup
		i2cWrite( ret );
	}

	sramStop();

	i2cWait();
	i2cStop();
}

//------------------------------------------------------------------------------
int __attribute__((noreturn)) main(void)
{
	DDRB = 0b00000111;
	DDRC = 0b00110001;
	DDRD = 0b11100000;
	PORTD = 0b01000000;

//	sramInit();
	
//	usbInit();
//	usbDeviceDisconnect();
//	_delay_ms( 250 );
//	usbDeviceConnect();

	i2cInit(10);
	oledInit();
	oledPowerOff();

	TCCR0B = 1<<CS01; // set 8-bit timer prescaler to /8 for 5859.375 intterupts per second @12mHz
	TIMSK0 = 1<<TOIE0; // fire off an interrupt every time it overflows, this is our tick (~170 microseconds per)

	rnaInit( 0x2 );

	sei();

	check = 0;
	unsigned char data[100];
	data[0] = 0x55;
	
	for(;;)
	{
		data[check]++;
		rnaSend( 0x1, data, 1 );

		_delay_ms(2);
	}
}

//------------------------------------------------------------------------------
ISR( TIMER0_OVF_vect )
{
//	usbPoll();
}
