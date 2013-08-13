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

#include "bootloader_code.h"

//------------------------------------------------------------------------------
int main()
{
	unsigned int pos = 0;
	unsigned int address = g_origin;
	unsigned char i;

	do
	{
		for( i=0; i<32; i++ )
		{
			if ( pos < g_size )
			{
				boot_page_fill( i, pgm_read_word(g_data[pos++]) );
			}
			else
			{
				boot_page_fill( i, 0 );
			}
		}

		boot_page_erase( address );
		boot_spm_busy_wait();
		boot_page_write( address );
		boot_spm_busy_wait();
		address += 64;
		
	} while( pos < g_size );

	asm	volatile ("ijmp" ::"z" (BOOTLOADER_ENTRY)); // jump to bootloader!
	return 0;
}
