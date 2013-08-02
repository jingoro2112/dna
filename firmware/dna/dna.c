#include "dna.h"
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
/*
	PORTB = 0b00001000; // turn on pullup and see if it has been shorted

	// give it a chance to stabalize and clock in, can't be too careful
	asm volatile( "nop" );
	asm volatile( "nop" );
	asm volatile( "nop" );
	asm volatile( "nop" ); 
	
	if ( !(PINB & 0b00001000) )
	{
		asm volatile ("ijmp" ::"z" (BOOTLOADER_ENTRY)); // jump to bootloader!
	}
*/
	asm volatile ("rjmp __ctors_end" ); // otherwise return to our regularly scheduled C program
}

