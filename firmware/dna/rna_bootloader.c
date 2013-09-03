/* Copyright: (c) 2013 by Curt Hartung
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/boot.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <util/delay.h>

#include <rna.h>

int g_addressBase; // page base currently being loaded
static int g_byteCountIn; // how much data is remaining to tranfer in the current code page
static int g_pageZero[32]; // retain page zero (ISR) in RAM, commit only after load is complete and validated
static int g_pageZeroIndex;
unsigned char g_codeChecksum; // must match before user app is entered
static unsigned char s_replyBuffer[4];

//------------------------------------------------------------------------------
void commitPage( unsigned int page )
{
	boot_page_erase( page );
	boot_spm_busy_wait();
	boot_page_write( page );
	boot_spm_busy_wait();
}

//------------------------------------------------------------------------------
unsigned char rnaInputSetup( unsigned char *data, unsigned char from, unsigned char len )
{
	if( data[0] == RNACommandAppJump )
	{
		asm	volatile ("ijmp" ::"z" (0x0)); // jump to bootloader!
	}
	else if ( data[0] == RNACommandCodePageWrite )
	{
		
	}
	
	return len;
}

//------------------------------------------------------------------------------
void rnaInputStream( unsigned char *data, unsigned char bytes )
{
}

//------------------------------------------------------------------------------
int __attribute__((noreturn)) main(void)
{
	// Once the bootloader has been entered, make sure it is always
	// entered until page zero is rewritten
	
	const int* isr = trampoline; // read array from program memory, saves copy step
	for( unsigned char i=0; i<sizeof(trampoline); i+=2 )
	{
		boot_page_fill( i, pgm_read_word(isr++) ); // rewrite the ISR to jump way up to our USB int0 routine
	}

	commitPage( 0 );

	// the bootloader is poll driven, so interrupt vectors need not be trampoline-hacked
	rnaInit();
	for(;;)
	{
		rnaPoll();
	}
}
