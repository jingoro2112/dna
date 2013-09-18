/* Copyright: (c) 2013 by Curt Hartung
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#include "dna.h"
#include <avr/io.h>
#include <util/delay.h>

//------------------------------------------------------------------------------
// this code is executed IMMEDIATELY out of reset, before any other C
// code has a chance to even set up the environment. It is important
// to trigger bootloader entry before anything else has the chance to
// send the MCU off into the weeds. Since the DNA board cannot be
// [easily] in-circuit reprogrammed.
//
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// ALTER/OMIT THIS AT YOUR OWN PERIL. THE DNA BOARD CANNOT BE SERIAL
// PROGRAMMED SINCE THE /RESET PIN HAS BEEN DISABLED! IF YOU BRICK THE
// BOARD IT WILL NEED TO BE REWORKED WITH A NEW CHIP OR HIGH-VOLTAGE
// PROGRAMMED
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
void __init()
{
	asm volatile ( "clr __zero_reg__" );

	// start the pin rising as quickly as possible
	DDRB = 0; // cheap insurance
	PORTB = 0b00001000; // turn on pullup

	// if the source of the reset was a watchdog timeout, indicating a software
	// request of bootloader entry, disable the watchdog and enter the
	// bootloader
	if ( WDTCSR & (1<<WDE) )
	{
		MCUSR = 0;
		WDTCSR |= (1<<WDCE) | (1<<WDE);
		WDTCSR = 0;
		goto bootloader_jump; // waste not, want not, this saves a few bytes (ijmp take 3 instructions, goto only 1)
	}

	// The logic is a little tortured; the reason is to put the
	// bootloader jump at the very end, since it's target location
	// will vary and therefore can't be brute-force checked like the
	// rest of this code by the loader

	_delay_us(50); // give state a chance to settle
	
	for( unsigned int i=0xFFFF; i; i-- )
	{
		// pin must be HELD low, make sure spurious RNA requests do not reset us!
		if ( PINB & 0b00001000 ) 
		{
			goto normalExit;
		}
	}
	
bootloader_jump:
	asm	volatile ("ijmp" ::"z" (BOOTLOADER_ENTRY)); // jump to bootloader!
	
normalExit:
	asm volatile ( "rjmp __ctors_end" ); // recovery not called for; return to the regularly scheduled C program
}

