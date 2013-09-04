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
	
	// give the pin a chance to stabilize and clock in, can't be too
	// careful, dont want to enter the bootloader accidentally, since
	// it is an irreversible action (it overwrites the vector table)
	_delay_us(50);
	if ( 0 )//!(PINB & 0b00001000) )
	{
bootloader_jump:
		asm	volatile ("ijmp" ::"z" (BOOTLOADER_ENTRY)); // jump to bootloader!
	}

	// The logic is a little tortured; the reason is to put this jump
	// at the very end, since it's target location will vary and
	// therefore can't be brute-force checked like the rest of this
	// code by the loader

	asm volatile ( "rjmp __ctors_end" ); // return to our regularly scheduled C program
}

