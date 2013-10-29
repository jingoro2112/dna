/*------------------------------------------------------------------------------*
 * Copyright: (c) 2013 by Curt Hartung avr@northarc.com
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#include <dna.h>
#include <cfal12832.h>
#include <i2c_brute.h>
#include <24c512.h>
#include <rna.h>
#include <rna_packet.h>
#include <sram.h>
#include <dstring.h>
#include <text.h>
#include <frame.h>

#include <util/delay.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>

#include "../eeprom_image.h"

volatile unsigned char rnaCommand;
volatile unsigned char rnaFrom;
volatile unsigned char rnaDataExpected;
volatile unsigned char rnaDataReceived;
unsigned char rnaPacket[sizeof(struct PacketEEPROMLoad) + 1];

//------------------------------------------------------------------------------
unsigned char rnaInputSetup( unsigned char *data, unsigned char dataLen, unsigned char from, unsigned char packetLen )
{
	rnaCommand = *data;
	rnaDataExpected = packetLen - 1;
	rnaDataReceived = 0;
	rnaFrom = from;
	
	if ( (rnaCommand == RNATypeEnterBootloader) && (dataLen == 1) )
	{
		wdt_enable( WDTO_15MS ); // light the fuse
	}
	
	return 1;
}

//------------------------------------------------------------------------------
void rnaInputStream( unsigned char *data, unsigned char bytes )
{
	for( unsigned char i=0; i<bytes; i++ )
	{
		rnaPacket[rnaDataReceived++] = *data++;
	}
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
		if ( PINB & 0b00000100 ) 
		{
			asm volatile ( "rjmp __ctors_end" ); // recovery not called for; return to the regularly scheduled C program
		}
	}

bootloader_jump:
	asm	volatile ("ijmp" ::"z" (OLED_BOOTLOADER_ENTRY)); // emergency reboot time
}

//------------------------------------------------------------------------------
int __attribute__((OS_main)) main()
{
	rnaInit();
	sei();

	sramInit();
	frameClear();
	
	i2cInit();
	oledInit( 0 );

	rnaDataExpected = 1;

	unsigned char c = 0xA5;
	unsigned char check = 0;
	write24c512( 0xA0, 0x1255, &c, 1 );
	read24c512( 0xA0, 0x1255, &check, 1 );

	if ( check == c )
	{
		stringAtResident( "EEPROM pass", 0, 0 );
	}
	else
	{
		stringAtResident( "EEPROM FAIL", 0, 0 );
	}

	check = 0;
	sramAtomicWrite( 0x1255, c );
	check = sramAtomicRead( 0x1255 );

	if ( check == c )
	{
		stringAtResident( "SRAM pass", 0, 8 );
	}
	else
	{
		stringAtResident( "SRAM FAIL", 0, 8 );
	}

	stringAtResident( "awaiting firmware image", 0, 16 );

	frameBlit();

	unsigned int errors = 0;

	for(;;)
	{
		if ( rnaDataReceived >= rnaDataExpected )
		{
			rnaDataReceived = 0;

			if ( rnaCommand == RNATypeEEPROMLoad )
			{
				struct PacketEEPROMLoad *load = (struct PacketEEPROMLoad *)rnaPacket;

				write24c512( 0xA0, load->offset, load->data, sizeof(load->data) );
				frameClear();
				
				char buf[128];
				dsprintf( buf, "heard block 0x%04X", load->offset );
				stringAtResident( buf, 0, 0 );

				read24c512( 0xA0, load->offset, (unsigned char *)buf, sizeof(load->data) );
				for( unsigned char i=0; i<sizeof(load->data); i++ )
				{
					if ( buf[i] != load->data[i] )
					{
						errors++;
					}
				}

				dsprintf( buf, "%d errors", errors );
				stringAtResident( buf, 0, 8 );
				
				frameBlit();
			}
		}
	}
}
