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

#define DOUBLE_CLICK_INTERVAL 150
#define POWER_OFF_INTERVAL 2500

//------------------------------------------------------------------------------
unsigned char rnaInputSetup( unsigned char *data, unsigned char dataLen, unsigned char from, unsigned char totalLen )
{
	if ( (*data == RNATypeEnterBootloader) && (dataLen == 1) )
	{
		wdt_enable( WDTO_15MS ); // light the fuse
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

	_delay_ms(1); // let values settle
	
	a2dStartConversion();
	a2dWaitForConversionComplete(); // throw out first reading

//	unsigned char high = 0xF0;
//	unsigned char low = 0xD0;

/*	
	unsigned char pos = 0;
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
	unsigned char status;

	unsigned char rnaPacket[3];
	rnaPacket[0] = RNATypeButtonStatus;
	rnaPacket[1] = 0;

	unsigned int cyclesOn = 0;
	unsigned int doubleClickStateCountdown[3] = { 0, 0, 0 };
	unsigned char doubleClickStateSent[3] = { 1, 1, 1 };
	unsigned char generateRelease = 0;

	for(;;)
	{
		if ( generateRelease )
		{
			generateRelease = 0;
			_delay_ms( 20 );
			rnaPacket[1] = 0;
			rnaSend( RNADeviceOLED, rnaPacket, 2 );
			_delay_ms( 20 );
		}

		status = 0;

		a2dStartConversion();
		a2dWaitForConversionComplete();

		// did we find a calibration?
		if ( ADCH < 0xE0 )
		{
			cyclesOn = 0; // power button not pressed, reset 'power off' logic and set state
			status |= ButtonBitMiddle;
		}
		if ( PINB & 0x1 )
		{
			status |= ButtonBitBottom;
		}

		if ( PINB & 0x2 )
		{
			status |= ButtonBitTop;
		}

		// the buttons are 'active low' but keep things
		// straightforward with a '1' meaning the button has been
		// pressed
		status ^= 0x7;

		for( unsigned char c = 0; c<3; c++ )
		{
			if( (status & (1<<c)) && !(oldStatus & (1<<c)) )
			{
				// just pressed
				if ( c == 1 )
				{
					if ( doubleClickStateCountdown[c] ) // and it's a double-click
					{
						doubleClickStateSent[c] = 1;
						rnaPacket[1] = (1<<c) | ButtonBitDouble;
						generateRelease = 1;
					}
					
					doubleClickStateCountdown[c] = 0;
				}
				else
				{
					rnaPacket[1] = (1<<c);
				}
			}
			else if ( !(status & (1<<c)) && (oldStatus & (1<<c)) )
			{
				// just released
				if ( c == 1 )
				{
					if ( doubleClickStateSent[c] )
					{
						doubleClickStateSent[c] = 0;
					}
					else
					{
						doubleClickStateCountdown[c] = DOUBLE_CLICK_INTERVAL;
					}
				}
				else
				{
					generateRelease = 1;
				}
			}
			else if ( doubleClickStateCountdown[c] )
			{
				if ( !--doubleClickStateCountdown[c] )
				{
					rnaPacket[1] = (1<<c);
					generateRelease = 1;
				}
			}
			
			// event generated? send it
			if ( rnaPacket[1] && rnaSend(RNADeviceOLED, rnaPacket, 2) == -1 )
			{
				_delay_ms(2);
				rnaSend( RNADeviceOLED, rnaPacket, 2 );
			}

			rnaPacket[1] = 0;
		}

		if ( oldStatus != status ) // debounce transitions
		{
//			rnaPacket[1] = status;
//			rnaSend( RNADeviceOLED, rnaPacket, 2 );
			oldStatus = status;
			_delay_ms( 20 );
		}
		else
		{
			_delay_ms( 1 );
		}
		
		if ( cyclesOn++ > POWER_OFF_INTERVAL )
		{
			rnaPacket[1] = ButtonBitPowerOff;
			rnaSend( RNADeviceOLED, rnaPacket, 2 );

			PORTB &= 0b11101111; // turn off the power pin (power won't actually die until the power button is released)
			for(;;);
		}
	}
}
