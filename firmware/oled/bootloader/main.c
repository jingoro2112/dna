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

//------------------------------------------------------------------------------
void commitPage( unsigned int page )
{
	boot_page_erase( page );
	boot_spm_busy_wait();
	boot_page_write( page );
	boot_spm_busy_wait();
}

//------------------------------------------------------------------------------
void processRnaPacket( unsigned char *data, unsigned char dataLen )
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

			if ( page->page < (OLED_BOOTLOADER_ENTRY*2) )  // silently ignore any command to overwrite myself
			{
				unsigned char pos = 0;
				for( unsigned char c=0; c<64; c += 2 )
				{
					boot_page_fill( c, page->code[pos++] );
				}

				commitPage( page->page );
			}
		}
		else if ( packet[0] == RNATypeEnterApp )
		{
			struct PacketEnterApp *enter = (struct PacketEnterApp *)(packet + 1);

			unsigned int crc = 0;
			for( unsigned int i=0; i<enter->lastAddress; i += 2 )
			{
				crc += pgm_read_word( i );
			}

			if ( crc == enter->checksum )
			{
				asm volatile ("ijmp" ::"z" (0)); // in with both feet...
			}
		}
	}
}

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
	processRnaPacket( data, dataLen );
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
// the process for figuring this out is rather ugly, after compiling,
// look in the .disassembly file for the __vector_1 symbol, then
// hand-compile an indirect call to it, and install that at the INT0
// interrupt vector, remembering to divide the address by 2 to get
// PC space, not address space!
const PROGMEM int isrJump[] =
{
	// trampoline to bootloader
	0xEAE0, // e0 ea  ldi r30, 0xA0       ; 160
	0xE0FE, // fe e0  ldi r31, 0x0E       ; 14
	0x9409, // 09 94  ijmp
};

//------------------------------------------------------------------------------
int __attribute__((OS_main)) main(void)
{
	const int* isr = isrJump; // read array from program memory, saves copy step
	for( unsigned char i=0; i<6; i+=2 )
	{
		boot_page_fill( i, pgm_read_word(isr++) ); // install RESET trampoline
	}
	commitPage( 0 );

	rnaInit();

	for(;;)
	{
		rnaPoll(); // everything else happens through the RNA bus
	}
}
