/*------------------------------------------------------------------------------*
 * Copyright: (c) 2013 by Curt Hartung avr@northarc.com
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#include <rna.h>
#include <rna_packet.h>
#include <dna_defs.h>
#include <a2d.h>

#include <avr/io.h>
#include <util/delay.h>
#include <avr/wdt.h>

#include <stdio.h>

unsigned char buttonRequest;

//------------------------------------------------------------------------------
unsigned char rnaInputSetup( unsigned char *data, unsigned char dataLen, unsigned char from, unsigned char totalLen )
{
	if ( (*data == RNATypeEnterBootloader) && (dataLen == 1) )
	{
		wdt_enable( WDTO_15MS ); // light the fuse
	}
	else if ( *data == RNATypeRequestButtonStatus )
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
// for software entry of bootloader, this is triggered with a watchdog reset
void __attribute__((OS_main)) __init()
{
	asm volatile ( "clr __zero_reg__" );

	// start the pin rising as quickly as possible
	DDRB = 0; // cheap insurance
	PORTB = 0b00000100; // turn on pullup for B2 (RNA bus)

	// if the source of the reset was a watchdog timeout, indicating a software
	// request of bootloader entry, disable the watchdog and enter the
	// bootloader
	if ( WDTCR & (1<<WDE) )
	{
		MCUSR = 0;

		WDTCR |= (1<<WDCE) | (1<<WDE);
		WDTCR = 0;
		goto bootloader_jump; // waste not, want not, this saves a few bytes (ijmp take 3 instructions, goto only 1)
	}

	// The logic is a little tortured; the reason is to put the
	// bootloader jump at the very end, since it's target location
	// will vary and therefore can't be brute-force checked like the
	// rest of this code by the loader

	_delay_us(50); // give state a chance to settle

	for( unsigned int i=5000; i; i-- )
	{
		_delay_ms(1);
		
		// pin must be HELD low, make sure spurious RNA requests do not reset us!
		if ( PINB & 0b00000100 ) 
		{
			asm volatile ( "rjmp __ctors_end" ); // recovery not called for; return to the regularly scheduled C program
		}
	}

bootloader_jump:
	asm	volatile ("ijmp" ::"z" (BUTTON_BOOTLOADER_ENTRY)); // emergency reboot time
}


//------------------------------------------------------------------------------
int __attribute__((OS_main)) main()
{
	// B0 top button (1)
	// B1 bottom button (3)
	// B2 RNA int0
	// B3 middle button
	// B4 power toggle
	DDRB = 0b00010000; // one output, the power toggle, everything else is an input
	PORTB = 0b00011011; // initial state: turn on pullups and keep the power on!

	rnaInit(); // interrupt driven, if we have the space it's the most convenient

	// configure the a2d to sample the middle button, which reads
	// nearly 1.1v for 'pressed' and about .8v for "not pressed" as a
	// result of the power-control circuitry design
	ADMUX = 1<<REFS1 | 1<<MUX0 | 1<<MUX1 | 1<<ADLAR; // B3, 1.1v ref, knock off lower two bits
	ADCSRA = 1<<ADEN; // enable A2D, x16
	DIDR0 = 1<<ADC3D; // disable all digital function on B3

	sei();

	unsigned char cyclesOn = 0;
	unsigned char pos = 0;

	_delay_ms(1); // let values settle
	
/*	
	_delay_ms( 500 ); // wait for everyone to boot before probing (the OLED in particular takes a few 100ms)
	unsigned int deltaTargets[0xE]; // when a button is pressed, everyone gets a notice
	for( unsigned char probe=1; probe<0x10; probe++ )
	{
		if ( rnaProbe(probe) )
		{
			deltaTargets[pos++] = probe;
		}
	}
	deltaTargets[pos] = 0;
*/

	unsigned char oldStatus = 0;

	unsigned char rnaPacket[3];
	rnaPacket[0] = RNATypeButtonStatus;
	
	for(;;)
	{
		a2dStartConversion();
		_delay_ms( 25 ); // wait for conversion plus debounce

		rnaPacket[1] = 0;
		if ( PINB & 0x1 )
		{
			rnaPacket[1] |= ButtonBitBottom;
		}

		if ( PINB & 0x2 )
		{
			rnaPacket[1] |= ButtonBitTop;
		}

		if ( ADCH < 0xE0 )
		{
			cyclesOn = 0; // power button not pressed, reset 'power off' logic and set state
			rnaPacket[1] |= ButtonBitMiddle;
		}

		// the buttons are 'active low' but keep things
		// straightforward with a '1' meaning the button has been
		// pressed
		rnaPacket[1] ^= 0x7; 

		if ( cyclesOn++ > 90 ) // more than 3 seconds held? power OFF
		{
			rnaPacket[1] = 0xFF;
			PORTB &= 0b11101111; // turn off the power pin (power won't actually die until the power button is released)
		}

		if ( rnaPacket[1] != oldStatus )
		{
			oldStatus = rnaPacket[1];

//			for( pos = 0; deltaTargets[pos]; pos++ )
//			{
//				while ( rnaSend(deltaTargets[pos], rnaPacket, 4) == -1 )
//				{
//					_delay_ms( 20 );
//				}
//			}

			if ( rnaSend( RNADeviceOLED, rnaPacket, 2 ) == -1 )
			{
				_delay_ms(20);
				rnaSend( RNADeviceOLED, rnaPacket, 2 );
			}
		}

		// board has asked status, go ahead and answer
		if ( buttonRequest )
		{
			rnaSend( buttonRequest, rnaPacket, 2 );
			buttonRequest = 0;
		}
		
		if ( rnaPacket[1] == 0xFF )
		{
			for(;;); // go quietly into the night
		}
	}
}
