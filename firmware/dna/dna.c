#include "dna.h"
#include <avr/io.h>
#include <util/delay.h>
#include <avr/wdt.h>
/* Copyright: (c) 2013 by Curt Hartung
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

//------------------------------------------------------------------------------
// this code is executed IMMEDIATELY out of reset, before any other C
// code has a chance to even set up the environment. This is important
// to inspect bootloader entry before anything else possibly screws up
// the MCU
//
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// ALTER/OMIT THIS AT YOUR OWN PERIL. THE DNA BOARD CANNOT BE SERIAL
// PROGRAMMED SINCE THE /RESET PIN HAS BEEN DISABLED! IF YOU BRICK THE
// BOARD IT WILL NEED TO BE REWORKED WITH A NEW CHIP
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
void __init()
{
	// start the pin rising as quickly as possible, if it's going to

	DDRB = 0; // juuust in case it has been screwed with (this is primarily
			  // to make sure this code still works in warm-reset situations
	PORTB = 0b00001000; // turn on pullup

	// some non-trivial code here, make sure c's assumptions are valid
	
//	asm volatile ( ".set __stack, %0" :: "i" (RAMEND) );  // not using the stack, but if we do, comment this back in!
	asm volatile ( "clr __zero_reg__" );
	
	// if the source of the reset was a watchdog timeout, indicating a software
	// request of bootloader entry, disable the watchdog and enter the
	// bootloader
	if ( WDTCSR & (1<<WDE) )
	{
		MCUSR = 0;
		WDTCSR |= (1<<WDCE) | (1<<WDE);
		WDTCSR = 0x00;
		goto bootloader_jump; // waste not, want not, this saves a few bytes (ijmp take 3 instructions, goto only 1)
	}
	
	// give the pin a chance to stabilize and clock in, can't be too careful
	_delay_us(50);
	if ( PINB & 0b00001000 )
	{
		asm volatile ("rjmp __ctors_end" ); // return to our regularly scheduled C program
	}

bootloader_jump:
	asm	volatile ("ijmp" ::"z" (BOOTLOADER_ENTRY)); // jump to bootloader!
}

