/* Copyright: (c) 2013 by Curt Hartung
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#include "../dna_defs.h"

#include <avr/io.h>
#include <avr/boot.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#include "bootloader_code.h"

//------------------------------------------------------------------------------
int __attribute__((OS_main)) main()
{
	cli();
	unsigned int address = g_origin;
	unsigned int pos = 0;
	const int* data = g_data;

	do
	{
		for( unsigned char i=0; i<64; i+=2 )
		{
			if ( pos < g_size )
			{
				boot_page_fill( i, pgm_read_word(data++) );
			}
			else
			{
				boot_page_fill( i, 0xFFFF );
			}

			pos++;
		}

		boot_page_erase( address );
		boot_spm_busy_wait();
		boot_page_write( address );
		boot_spm_busy_wait();
		address += 64;
		
	} while( pos < g_size );

	asm	volatile ("ijmp" ::"z" (g_origin/2)); // jump to bootloader we just loaded
}
