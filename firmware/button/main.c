/* Copyright: (c) 2013 by Curt Hartung
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#include <rna.h>
#include <avr/io.h>
#include <util/delay.h>

#define On()  (PORTB |= 0b00000001)
#define Off() (PORTB &= 0b11111110)

// B0 annunciator [top button]
// B1 bottom button
// B2 RNA int0
// B3 middle button
// B4 power toggle

#define bottomButton() (PINB & 0b00000010)

enum Messages
{
	bottomButtonUp = 1,
	bottomButtonDown,
};
  
unsigned char data;

//------------------------------------------------------------------------------
unsigned char rnaInputSetup( unsigned char *data, unsigned char from, unsigned char len )
{
//	d = data[0];// + 1;
//	rnaSend( from, &d, 1 );
	return 1;
}

//------------------------------------------------------------------------------
void rnaInputStream( unsigned char *data, unsigned char bytes )
{
	
}

//------------------------------------------------------------------------------
int __attribute__((OS_main)) main()
{
	// set up I/O
	DDRB  = 0b00010001;
	PORTB = 0b00011010; // keep the power on!

	rnaInit();
	sei();

	unsigned char buf[20];
	buf[0] = 0x55;

	unsigned char lastBottomStatus = bottomButton();
	
	for(;;)
	{
		_delay_us( 20 );
		if ( bottomButton() != lastBottomStatus )
		{
			On();
			lastBottomStatus = bottomButton();
			if ( lastBottomStatus )
			{
				data = bottomButtonUp;
			}
			else
			{
				data = bottomButtonDown;
			}
			rnaSend( RNAAddressDNA, &data, 1 );
			Off();
		}
	}
}
