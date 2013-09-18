#include "oled.h"
/* Copyright: (c) 2013 by Curt Hartung
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#include <util/delay.h>
#include <avr/io.h>

#include "i2c_interface.h"

//------------------------------------------------------------------------------
// ineffcient in time, yes, small code space making this
// self-contained. When single commands are being sent they are plenty fast enough. 
void oledCommand( unsigned char cmd )
{
	i2cStartWrite( OLED_ADDRESS );
	i2cWrite( 0x00 ); // single command
	i2cWrite( cmd );
	i2cStop();
}

//------------------------------------------------------------------------------
// clear the display as quickly as possible
void oledClear()
{
	i2cStartWrite( OLED_ADDRESS );

	// theoretically this should not be required, since the display is
	// always written as a full-screen, if the application gets truly
	// under the size-crunch
	// select start of row, first page
	i2cWrite( 0x80 );
	i2cWrite( 0x00 ); 
	i2cWrite( 0x80 );
	i2cWrite( 0x10 );
	i2cWrite( 0x80 );
	i2cWrite( 0xB0 );

	// clock in 512 zeros
	unsigned int i;
	for( i=0; i<512; i++ )
	{
		i2cWrite( 0xC0 );
		i2cWrite( 0 );
	}

	i2cStop();
}

//------------------------------------------------------------------------------
void oledInit()
{
	OLED_CS_DDR |= 1<<OLED_CS_PIN_NUMBER;
	OLED_RESET_DDR |= 1<<OLED_RESET_PIN_NUMBER;

	oledCSLow();

	oledResetLow();
	_delay_ms( 100 );
	oledResetHigh();

	oledCommand( 0xAE ); //--Set Display off

//	oledCommand( 0xC0 ); // bottom to top
	oledCommand( 0xC8 ); // top to bottom
//	oledCommand( 0xA0 ); // right to left
	oledCommand( 0xA1 ); // left to right
	
	oledCommand( 0x20 );
	oledCommand( 0x00 );

	oledCommand( 0x00 ); //--set low column address
	oledCommand( 0x10 ); //--set high column address

	oledCommand( 0x81 ); //--set contrast control register
	oledCommand( 0x7F);

	oledCommand( 0xA6 ); //--set normal display

	oledCommand( 0xA8 ); //--set multiplex ratio(1 to 16)
	oledCommand( 0x1F ); //--duty 1/32

	oledCommand( 0xD3 ); //--set display offset
	oledCommand( 0x00 ); //--not offset

	oledCommand( 0xD5 ); //--set display clock divide ratio/oscillator frequency
	oledCommand( 0xF0 ); //--set divide ratio

	oledCommand( 0xD9 ); //--set pre-charge period
	oledCommand( 0x22);

	oledCommand( 0xDA ); //--set com pins hardware configuration
	oledCommand( 0x02 ); //disable left/right remap and set for sequential

	oledCommand( 0xDB ); //--set vcomh
	oledCommand( 0x49 ); //--0.83*vref

	oledCommand( 0x8D ); //--set DC-DC enable
	oledCommand( 0x14 ); //

	oledCommand( 0xAF ); //--turn on oled panel

	oledCommand( 0xA4 ); //--Entire Display ON

	oledClear();
}


