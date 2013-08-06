/* Copyright: (c) 2013 by Curt Hartung
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#include <dna.h>
#include <usb.h>
#include <oled.h>
#include <i2c.h>
#include <24c512.h>

#include "sram.h"

#include <util/delay.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#define ledOn()  (PORTB &= 0b11111110)
#define ledOff() (PORTB |= 0b00000001)


volatile unsigned char oledBusy = 0;

unsigned char d[8];
//------------------------------------------------------------------------------
unsigned char dnaUsbInputSetup( unsigned char *data, unsigned char len )
{
	switch( data[0] )
	{
		case 1:
		{
			d[0] = 0xA1;
			d[1] = 0xB2;
			d[2] = 0xC1;
			d[3] = 0xD1;
			while( oledBusy );
			oledBusy = 1;
			sramWrite( 120, d[0] );
			sramWrite( 130, d[1] );
			sramWrite( 140, d[2] );
			sramWrite( 150, d[3] );
			oledBusy = 0;

//			write24c512( 0xA0, 100, d, 4 );
//			write24c512( 0xA0, 101, d+1, 1 );
//			write24c512( 0xA0, 102, d+2, 1 );
//			write24c512( 0xA0, 103, d+3, 1 );
			break;
		}

		case 2:
		{
			d[0] = 1;
			d[1] = 2;
			d[2] = 3;
			d[3] = 4;
			while( oledBusy );
			oledBusy = 1;
			d[0] = sramRead( 120 );
			d[1] = sramRead( 130 );
			d[2] = sramRead( 140 );
			d[3] = sramRead( 150 );
			oledBusy = 0;
//			read24c512( 0xA0, 100, d, 4 );
//			read24c512( 0xA0, 101, d+1, 1 );
//			read24c512( 0xA0, 102, d+2, 1 );
//			read24c512( 0xA0, 103, d+3, 1 );
			dnaUsbQueueData( d, 4 );
			
			break;
		}
	}


	return 0;
}

//------------------------------------------------------------------------------
void dnaUsbInputStream( unsigned char *data, unsigned char len )
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

	while( oledBusy );
	oledBusy = 1;

	sramStartRead( address );

	unsigned int i;
	unsigned char ret;
	for( i=0; i<512; i++ )
	{
		ret = sramClockOutByte();
		i2cWriteNoWait( 0xC0 ); // data setup
		i2cWriteNoWait( ret );
	}

	sramStop();

	i2cWait();
	i2cStop();
	
	oledBusy = 0;
}

//------------------------------------------------------------------------------
int __attribute__((noreturn)) main(void)
{
	DDRB = 0b00000111;
	DDRC = 0b00110001;
	DDRD = 0b10100000;
	PORTD = 0b01000000;

	sramInit();
	
	ledOff();

	i2cInit( 10 );

	usbInit();
	
	usbDeviceDisconnect();
	_delay_ms( 250 );
	usbDeviceConnect();

	oledInit();
//	oledPowerOff();

	TCCR0B = 1<<CS01; // set 8-bit timer prescaler to /8 for 5859.375 intterupts per second @12mHz
	TIMSK0 = 1<<TOIE0; // fire off an interrupt every time it overflows, this is our tick (~170 microseconds per)

	sei();

	unsigned char counter;
	for(;;)
	{
		ledOn();
		sram2oled( counter );
		counter ++;
		ledOff();
		_delay_ms(10);
/*
		sram2oled( 0x0 );
		ledOn();
//		_delay_ms(1000);
		oledClear();
		ledOff();
//		_delay_ms(1000);
		
//		show_bitmap( counter++ );
//		*/
	}
}

//------------------------------------------------------------------------------
ISR( TIMER0_OVF_vect )
{
	usbPoll();
}
