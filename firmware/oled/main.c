/*------------------------------------------------------------------------------*
 * Copyright: (c) 2013 by Curt Hartung avr@northarc.com
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#include <dna.h>
#include <oled.h>
#include <i2c_brute.h>
#include <24c512.h>
#include <rna.h>
#include <rna_packet.h>
#include <sram.h>
#include <galloc.h>

#include <util/delay.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#include <stdio.h>

#include "../../oled/font_loader.h"
#include "frame.h"

unsigned char rnaCommand;
unsigned char rnaFrom;
unsigned char rnaDataExpected;
unsigned char rnaDataReceived;
unsigned char rnaPacket[132];

unsigned char numberOfFonts;
unsigned int dataBlockOrigin;

#define NUMBER_OF_BIT_ENTRIES 2
struct Bits
{
	volatile uint8 b0:1;
	volatile uint8 b1:1;
	volatile uint8 b2:1;
	volatile uint8 b3:1;
	volatile uint8 b4:1;
	volatile uint8 b5:1;
	volatile uint8 b6:1;
	volatile uint8 b7:1;
} volatile bits[NUMBER_OF_BIT_ENTRIES];

#define displayDirty		(bits[0].b0)

#define SRAM_DEBUG_LINE_BUFFER_START 512
#define SRAM_DEBUG_LINE_BUFFER_WIDTH 30
#define SRAM_DEBUG_LINE_BUFFER_LINES 4
#define SRAM_DEBUG_LINE_FONT 0
#define SRAM_DEBUG_LINE_SPACING 8

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
void blit()
{
	i2cStartWrite( OLED_ADDRESS );
	i2cWrite( 0x80 ); // command setup
	i2cWrite( 0x00 );
	i2cWrite( 0x80 ); // command setup
	i2cWrite( 0x10 );
	i2cWrite( 0x80 ); // command setup
	i2cWrite( 0xB0 );

	sramStartRead( 0 ); // oth location is the frame buffer

	for( unsigned char i=0; i<0xFF; i++ )
	{
		i2cWrite( 0xC0 );
		i2cWrite( sramReadByte() );
		i2cWrite( 0xC0 );
		i2cWrite( sramReadByte() );
	}

	sramStop();
	i2cWait();
	i2cStop();
}

//------------------------------------------------------------------------------
static uint16 computeAddress( unsigned char x, unsigned char y )
{
	return ((y & 0xF8) << 4) + x;
}

//------------------------------------------------------------------------------
void resetPixelFrame( unsigned char x, unsigned char y )
{
	if ( x < 128 && y < 32 )
	{
		uint16 address = computeAddress( x, y );
		sramAtomicWrite( address, sramAtomicRead(address) & ~(1 << (y & 0x07)) );
	}
}

//------------------------------------------------------------------------------
void setPixelFrame( unsigned char x, unsigned char y )
{
	if ( x < 128 && y < 32 )
	{
		uint16 address = computeAddress( x, y );
		sramAtomicWrite( address, sramAtomicRead(address) | (1 << (y & 0x07)) );
	}
}

//------------------------------------------------------------------------------
void setPixelLive( unsigned char x, unsigned char y )
{
	if ( x < 128 && y < 32 )
	{
		uint16 address = computeAddress( x, y );
		uint8 newVal = sramAtomicRead(address) | (1 << (y & 0x07));

		sramAtomicWrite( address, newVal );

		i2cStartWrite( OLED_ADDRESS );
		i2cWrite( 0x80 );
		i2cWrite( x & 0x0F );
		i2cWrite( 0x80 );
		i2cWrite( x>>4 | 0x10 );
		i2cWrite( 0x80 );
		i2cWrite( 0xB0 | y>>3 );

		i2cWrite( 0xC0 );
		i2cWrite( newVal );

		i2cStop();
	}
}

//------------------------------------------------------------------------------
void resetPixelLive( unsigned char x, unsigned char y )
{
	if ( x < 128 && y < 32 )
	{
		uint16 address = computeAddress( x, y );
		uint8 newVal = sramAtomicRead(address) & ~(1 << (y & 0x07));
		sramAtomicWrite( address, newVal );

		i2cStartWrite( OLED_ADDRESS );
		i2cWrite( 0x80 );
		i2cWrite( x & 0x0F );
		i2cWrite( 0x80 );
		i2cWrite( x>>4 | 0x10 );
		i2cWrite( 0x80 );
		i2cWrite( 0xB0 | y>>3 );

		i2cWrite( 0xC0 );
		i2cWrite( newVal );

		i2cStop();
	}
}

//------------------------------------------------------------------------------
void debugPrint( char *string )
{
	unsigned char i;
	unsigned int address = SRAM_DEBUG_LINE_BUFFER_START;

	for( unsigned char c=1; c<SRAM_DEBUG_LINE_BUFFER_LINES; c++ )
	{
		for( i=0; i<SRAM_DEBUG_LINE_BUFFER_WIDTH; i++ )
		{
			sramAtomicWrite( address, sramAtomicRead(address + SRAM_DEBUG_LINE_BUFFER_WIDTH) );
			address++;
		}
	}

	sramStartWrite( address );
	for( i=0; i<SRAM_DEBUG_LINE_BUFFER_WIDTH - 1; i++ )
	{
		sramWriteByte( string[i] );
		if ( !string[i] )
		{
			break;
		}
	}

	for(; i<SRAM_DEBUG_LINE_BUFFER_WIDTH; i++ )
	{
		sramWriteByte( 0 );
	}
	address += SRAM_DEBUG_LINE_BUFFER_WIDTH;
	sramStop();

	displayDirty = 1;
}

//------------------------------------------------------------------------------
void stringAtEmbedded( char *string, unsigned char x, unsigned char y, unsigned char font )
{
	for( ;*string ;string++ )
	{
		unsigned int offset = *string - 32;
		offset *= sizeof(struct FontCharEntry) * NUMBER_OF_FONTS;
		offset += sizeof(struct FontCharEntry) * (unsigned int)font;

		struct FontCharEntry entry;
		for( unsigned int i=0; i<sizeof(struct FontCharEntry); i++ )
		{
			((unsigned char*)&entry)[i] = pgm_read_byte( c_lookupTable + offset + i );
		}

		x += entry.pre;

		const unsigned char* pos = c_dataBlock + entry.dataOffset;
		unsigned char byte = 0;
		unsigned char bit = 1;
		for( unsigned char h=0; h<entry.h; h++ )
		{
			if ( bit )
			{
				byte = pgm_read_byte( pos++ );
				bit = 0;
			}

			for( unsigned char w = 0; w<entry.w; w++ )
			{
				if ( 1<<bit & byte )
				{
					setPixelFrame( x + w, y + h );
				}

				if ( ++bit == 8 )
				{
					byte = pgm_read_byte( pos++ );
					bit = 0;
				}
			}
		}
		
		x += entry.post + entry.w;
	}
}

//------------------------------------------------------------------------------
void stringAt( char* string, unsigned char x, unsigned char y, unsigned char font )
{
	for( ;*string ;string++ )
	{
		unsigned int offset = ((unsigned int)(*string - 32) * (sizeof(struct FontCharEntry) * (unsigned int)numberOfFonts)) // which character
							  + (sizeof(struct FontCharEntry) * (unsigned int)font) // which font we want
							  + 1; // numberof fonts location
		
		struct FontCharEntry entry;
		read24c512( 0xA0, offset, (unsigned char*)&entry, sizeof(struct FontCharEntry) );

		x += entry.pre;

		unsigned char glyph[64];
		unsigned char *byte = glyph;
		read24c512( 0xA0, entry.dataOffset + dataBlockOrigin, glyph, 64 );

		for( unsigned char h=0; h<entry.h; h++ )
		{
			unsigned char bit = 0;
			for( unsigned char w = 0; w<entry.w; w++ )
			{
				if ( 1<<bit & *byte )
				{
					setPixelFrame( x + w, y + h );
				}

				if ( ++bit == 8 )
				{
					byte++;
					bit = 0;
				}
			}

			if ( bit )
			{
				byte++;
			}
		}

		x += entry.post + entry.w;
	}
}

//------------------------------------------------------------------------------
void fetchEEPROMString( unsigned int string, char* buf )
{
	
}

//------------------------------------------------------------------------------
void printAt( unsigned int string, unsigned char x, unsigned char y )
{
	
}

//------------------------------------------------------------------------------
void clearFrameBuffer()
{
	sramStartWrite( 0 );
	
	for( unsigned char i=0; i<0xFF; i++ )
	{
		sramWriteByte( 0 );
		sramWriteByte( 0 );
	}

	sramStop();
}

//------------------------------------------------------------------------------
void programEEPROM()
{
	stringAtEmbedded( "EEPROM Update", 0, 0, 0 );
	blit();
	unsigned int address = 1;

	startWrite24c512( 0xA0, 0 );
	streamWrite24c512( NUMBER_OF_FONTS );

	unsigned int i;
	for( i=0; i<FONT_BLOCK_SIZE; i++ )
	{
		if ( !(address%128) )
		{
			stop24c512();
			startWrite24c512( 0xA0, address );
		}

		streamWrite24c512( pgm_read_byte(c_lookupTable + i) );
		address++;
	}

	for( i=0; i<DATA_BLOCK_SIZE; i++ )
	{
		if ( !(address%128) )
		{
			stop24c512();
			startWrite24c512( 0xA0, address );
		}

		streamWrite24c512( pgm_read_byte(c_dataBlock + i) );
		address++;
	}
	stop24c512();
	clearFrameBuffer();
	stringAtEmbedded( "complete", 20, 0, 0 );
	blit();
	_delay_ms(500);
	clearFrameBuffer();
	blit();
}

//------------------------------------------------------------------------------
void setupVariables()
{
	// clear bits
	for( unsigned char c = 0; c<NUMBER_OF_BIT_ENTRIES; c++ )
	{
		*(((unsigned char*)&bits) + c) = 0;
	}

	// clear SRAM
	sramStartWrite( 0 );
	for( unsigned int a=0; a<8192; a++ )
	{
		sramWriteByte( 0 );
	}
	sramStop();

	rnaDataExpected = 1; // make sure nothing triggers
}

//------------------------------------------------------------------------------
int __attribute__((OS_main)) main()
{
	sramInit();
	i2cInit();
	oledInit( 1 );
	rnaInit();

	clearFrameBuffer();
	blit();

	setupVariables();

//	programEEPROM();

	// read in font table
	read24c512( 0xA0, 0, &numberOfFonts, 1 );
	dataBlockOrigin = ((unsigned int)numberOfFonts * sizeof(struct FontCharEntry) * 95) + 1;

	debugPrint( "0 DNA ready" );
//	debugPrint( "0:<1>DNA ready" );
//	debugPrint( "0:<2>DNA ready" );
//	debugPrint( "0:<3>DNA ready" );
//	debugPrint( "0:<4>DNA ready" );
//	debugPrint( "0:<5>DNA ready" );


	sei();

	unsigned int packetNum = 0;
	
	unsigned char frameMode = displayDebug;

	for(;;)
	{
		if ( displayDirty )
		{
			displayDirty = 0;
			clearFrameBuffer();
			
			if ( frameMode == displayDebug )
			{
				unsigned int address = SRAM_DEBUG_LINE_BUFFER_START;
				unsigned char vertical = 0;
				unsigned char cursor = 0;
				char string[SRAM_DEBUG_LINE_BUFFER_WIDTH + 1];
				
				for ( unsigned char line = 0; line < SRAM_DEBUG_LINE_BUFFER_LINES; line++ )
				{
					for ( cursor = 0; cursor < SRAM_DEBUG_LINE_BUFFER_WIDTH; cursor++ )
					{
						string[cursor] = sramAtomicRead( address++ );
					}

					string[cursor] = 0;
					stringAt( string, 0, vertical, SRAM_DEBUG_LINE_FONT );
					vertical += SRAM_DEBUG_LINE_SPACING;
				}	
			}

			blit();
		}

		if ( rnaDataReceived >= rnaDataExpected )
		{
			rnaDataReceived = 0;
			char buf[32];
			
			if ( rnaCommand == RNATypeEEPROMLoad )
			{
				struct PacketEEPROMLoad *load = (struct PacketEEPROMLoad *)rnaPacket;

				sprintf( buf, "%04X:%d", (unsigned int)load->offset, packetNum++ );
				debugPrint( buf );

				startWrite24c512( 0xA0, load->offset );
				for ( unsigned int i=0; i<128; i++ )
				{
					i2cWrite( load->data[i] );
				}
				i2cStop();

				if ( load->offset == 0 )
				{
					numberOfFonts = load->data[0];
					dataBlockOrigin = ((unsigned int)numberOfFonts * sizeof(struct FontCharEntry) * 95) + 1;
				}
			}
			else if ( rnaCommand == RNATypeDebugString )
			{
				rnaPacket[31] = 0; // enforce termination
				sprintf( buf, "%d %s", rnaFrom, rnaPacket );

				debugPrint( buf );
			}
			else if ( rnaCommand == RNATypeButtonStatus )
			{
				sprintf( buf, "%d %s %s %s",
						 rnaFrom,
						 rnaPacket[0] & 0x1 ? "on " : "off",
						 rnaPacket[0] & 0x2 ? "on " : "off",
						 rnaPacket[0] & 0x4 ? "on " : "off" );

				if ( rnaPacket[0] == 0xFF )
				{
					sprintf( buf, "3: POWER OFF!!!" );
				}

				debugPrint( buf );
			}
		}
	}
}
