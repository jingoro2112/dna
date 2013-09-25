/*------------------------------------------------------------------------------*
 * Copyright: (c) 2013 by Curt Hartung avr@northarc.com
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#include <rna.h>
#include <dna_defs.h>
#include <a2d.h>

#include <avr/io.h>
#include <util/delay.h>

#include "button.h"

unsigned char buttonRequest;

//------------------------------------------------------------------------------
unsigned char rnaInputSetup( unsigned char *data, unsigned char dataLen, unsigned char from, unsigned char totalLen )
{
	if ( *data == ButtonRequestPowerOff )
	{
		PORTB &= 0b11101111; // turn off the power pin
	}
	else
	{
		buttonRequest = from; // echo the button status
	}
	
	return 1;
}

//------------------------------------------------------------------------------
void rnaInputStream( unsigned char *data, unsigned char dataLen )
{
}

//------------------------------------------------------------------------------
int __attribute__((OS_main)) main()
{
	// B0 top button (1)
	// B1 bottom button (3)
	// B2 RNA int0
	// B3 middle button
	// B4 power toggle
	DDRB  = 0b00010000; // one output, the power toggle
	PORTB = 0b00011011; // turn on pullups and keep the power on!

	
	// set up A2D
	ADMUX = 1<<REFS1 | 1<<MUX0 | 1<<MUX0 | 1<<ADLAR; // B3, 1.1v ref, knock off lower two bits
	ADCSRA = 1<<ADEN | 1<<ADPS2; // enable A2D, x16
	DIDR0 = ADC3D; // disable all digital function on B3

	rnaInit();

	unsigned char buttonStatus;
	unsigned int cyclesOn;

	// sample continuously so the power can be turned off independantly
	for(;;)
	{
		a2dStartConversion();
		_delay_us(250);
		a2dWaitForConversionComplete();
		if ( ADCH > 0x20 )
		{
			buttonStatus = (PINB & 0x1) | ((PINB & 0x2) << 1) | 0x2; // power pressed
		}
		else
		{
			cyclesOn = 0;
			buttonStatus = (PINB & 0x1) | ((PINB & 0x2) << 1); // power not pressed
		}

		if ( buttonRequest )
		{
			rnaSend( buttonRequest, &buttonStatus, 1 );
			buttonRequest = 0;
		}
		
		if ( cyclesOn++ > 12000 ) // more than 3 seconds held? power OFF
		{
			PORTB &= 0b11101111; // turn off the power pin
		}
	}
}
