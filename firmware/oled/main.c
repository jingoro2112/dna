/* Copyright: (c) 2013 by Curt Hartung
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#include <dna.h>
#include <oled.h>
#include <i2c_brute.h>
#include <24c512.h>
#include <rna.h>
#include <sram.h>

#include <util/delay.h>
#include <avr/io.h>
#include <avr/interrupt.h>

//------------------------------------------------------------------------------
unsigned char rnaInputSetup( unsigned char *data, unsigned char from, unsigned char len )
{
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

//	sramStartRead( address );

	unsigned int i;
	unsigned char ret;
	static char off = 0;
	off++;
	for( i=0; i<512; i++ )
	{
		ret = i+off;//sramReadByte();
		i2cWrite( 0xC0 ); // data setup
		i2cWrite( ret );
	}

//	sramStop();
	i2cWait();
	i2cStop();
}

//------------------------------------------------------------------------------
int __attribute__((OS_main)) main()
{
	sramInit();
	i2cInit();
	
	oledInit();
	oledPowerOff();
//	rnaInit();

//	sei();

	unsigned char byte[5] = { 0xAA, 0x55, 0xAA, 0x55, 0xAA };
	
	for(;;)
	{
//		sramAtomicWrite( 0x1234, 0x51 );
//		_delay_ms( 1 );
//		sramAtomicRead( 0x1234 );
		
//		write24c512( 0xA0, 0x1234, byte, 2 );
//		_delay_ms( 2 );
//		read24c512( 0xA0, 0x1234, byte, 2 );


//		sram2oled(0); // 0 page is the defualt fram buffer
//		_delay_ms( 500 );


	}
}

