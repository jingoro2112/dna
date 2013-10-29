/*------------------------------------------------------------------------------*
 * Copyright: (c) 2013 by Curt Hartung avr@northarc.com
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#include <dna.h>
#include <rna.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/boot.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <rna_packet.h>

#include "../../dna/dna_types.h"

uint8 packet[128];
uint8 packetExpectedLength;
uint8 packetPos;
uint8 jumpToApp;

//------------------------------------------------------------------------------
unsigned char rnaInputSetup( unsigned char *data, unsigned char dataLen, unsigned char from, unsigned char packetLen )
{
	packetPos = 0;
	packetExpectedLength = packetLen;
	return 0;
}

//------------------------------------------------------------------------------
void rnaInputStream( unsigned char *data, unsigned char dataLen )
{
	for( unsigned char i=0; i<dataLen; i++ )
	{
		packet[packetPos++] = data[i];
	}

	if ( packetPos >= packetExpectedLength )
	{
		if ( packet[0] == RNATypeCodePage )
		{
			struct PacketCodePage *page = (struct PacketCodePage *)(packet + 1);

			unsigned char pos = 0;
			for( unsigned char c=0; c<64; c += 2 )
			{
				boot_page_fill( c, page->code[pos++] );
			}

			boot_page_erase( page->page );
			boot_spm_busy_wait();
			boot_page_write( page->page );
			boot_spm_busy_wait();
		}
		else if ( packet[0] == RNATypeEnterApp )
		{
			jumpToApp = 1;
		}
	}
}

// manually locate the reset vector, since we took out the automatic
// table generation in the custom linking file
//------------------------------------------------------------------------------
void ResetVector (void) __attribute__ ((naked)) __attribute__ ((section (".reset"))); 
void ResetVector (void) 
{ 
	asm volatile ("rjmp __ctors_end");
}

//------------------------------------------------------------------------------
int __attribute__((OS_main)) main(void)
{
	DDRD = 0x0;
	PORTD = 0b00000100; // turn on pullup for D2 (RNA bus)

	// if the source of the reset was a watchdog timeout, indicating a software
	// request of bootloader entry, disable the watchdog and enter the
	// bootloader
	if ( WDTCSR & (1<<WDE) )
	{
		MCUSR = 0;
		WDTCSR |= (1<<WDCE) | (1<<WDE);
		WDTCSR = 0;
	}
	else
	{
		_delay_us(50); // give state a chance to settle

		for( unsigned int i=4000; i; i-- )
		{
			_delay_ms(1);
			// pin must be HELD low, make sure spurious RNA requests do not reset us!
			if ( PIND & 0b00000100 ) 
			{
				goto jumpToZero;
			}
		}
	}

	rnaInit();

	for(;;)
	{
		rnaPoll(); // everything else happens through the RNA bus

		if ( jumpToApp )
		{
jumpToZero:
			boot_rww_enable();
			asm volatile ("ijmp" ::"z" (0)); // in with both feet...
		}
	}
}
