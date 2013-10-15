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
#include <dstring.h>

#include <stdio.h>

#include <util/delay.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>

#include "../../oled/eeprom_image.h"
#include "frame.h"
#include "text.h"
#include "menu.h"

#define true 1
#define false 0


//------------------------------------------------------------------------------
enum
{
	EEMenuBlobAddress = 0x0,

	EEBootStatus = 0x80,
};

//------------------------------------------------------------------------------
enum
{
	BootStatusInitial = 0xFF,
};

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

#define SRAM_CONSOLE_LINE_BUFFER_START 512
#define SRAM_CONSOLE_LINE_BUFFER_WIDTH 30
#define SRAM_CONSOLE_LINE_BUFFER_LINES 5
#define SRAM_CONSOLE_LINE_FONT 5
#define SRAM_CONSOLE_LINE_SPACING 6
unsigned char currentConsoleLine;

volatile unsigned char rnaCommand;
volatile unsigned char rnaFrom;
volatile unsigned char rnaDataExpected;
volatile unsigned char rnaDataReceived;
unsigned char rnaPacket[132];

unsigned char numberOfFonts;
unsigned int dataBlockOrigin;

unsigned char frameMode;

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

	i2cWrite( 0x80 );
	i2cWrite( 0x00 );
	
	i2cWrite( 0x80 );
	i2cWrite( 0x10 );
	
	i2cWrite( 0x80 );
	i2cWrite( 0xB0 );

	sramStartRead( 0 ); // 0th location is the frame buffer

	for( unsigned char i=0; i<0x80; i++ )
	{
		i2cWrite( 0xC0 );
		i2cWrite( sramReadByte() );
		i2cWrite( 0xC0 );
		i2cWrite( sramReadByte() );
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
void resetPixel( char x, char y )
{
	if ( (unsigned char)x < 128 && (unsigned char)y < 32 )
	{
		uint16 address = computeAddress( x, y );
		sramAtomicWrite( address, sramAtomicRead(address) & ~(1 << (y & 0x07)) );
	}
}

//------------------------------------------------------------------------------
void setPixel( char x, char y )
{
	if ( (unsigned char)x < 128 && (unsigned char)y < 32 )
	{
		uint16 address = computeAddress( x, y );
		sramAtomicWrite( address, sramAtomicRead(address) | (1 << (y & 0x07)) );
	}
}

//------------------------------------------------------------------------------
void setPixelLive( char x, char y )
{
	if ( (unsigned char)x < 128 && (unsigned char)y < 32 )
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
void resetPixelLive( char x, char y )
{
	if ( (unsigned char)x < 128 && (unsigned char)y < 32 )
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

#define consolePrint( string ) consolePrintEx(0, string)
#define consolePrint_P( string ) consolePrintEx(1, string)
//------------------------------------------------------------------------------
void consolePrintEx( unsigned char flash, char *string )
{
	sramStartWrite( SRAM_CONSOLE_LINE_BUFFER_START + (currentConsoleLine * SRAM_CONSOLE_LINE_BUFFER_WIDTH) );
	
	for( unsigned char c=0; string[c] && c<(SRAM_CONSOLE_LINE_BUFFER_WIDTH - 1); c++ )
	{
		sramWriteByte( string[c] );
	}
	sramWriteByte( 0 );
	sramStop();

	if ( ++currentConsoleLine >= SRAM_CONSOLE_LINE_BUFFER_LINES )
	{
		currentConsoleLine = 0;
	}
}

//------------------------------------------------------------------------------
void clearFrameBuffer()
{
	sramStartWrite( 0 );
	
	for( unsigned char i=0; i<0x80; i++ )
	{
		sramWriteByte( 0 );
		sramWriteByte( 0 );
		sramWriteByte( 0 );
		sramWriteByte( 0 );
	}

	sramStop();
}

//------------------------------------------------------------------------------
void init()
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
		sramWriteByte( 0x0 );
	}
	sramStop();
	rnaDataExpected = 1; // make sure nothing triggers

	frameMode = FrameConsole;
//	displayDirty = true;

	clearFrameBuffer();
	blit();

	// read in font table
	read24c512( 0xA0, 0, &numberOfFonts, 1 );
	dataBlockOrigin = ((unsigned int)numberOfFonts * sizeof(struct FontCharEntry) * 95) + 1;
}

//------------------------------------------------------------------------------
void render()
{
	if ( frameMode == FrameConsole )
	{
		char out[SRAM_CONSOLE_LINE_BUFFER_WIDTH];
		unsigned char line = currentConsoleLine;
		unsigned char vertical = 0;
		for( unsigned char l = 0; l<SRAM_CONSOLE_LINE_BUFFER_LINES; l++ )
		{
			sramStartRead( SRAM_CONSOLE_LINE_BUFFER_START + (line * SRAM_CONSOLE_LINE_BUFFER_WIDTH) );
			for( unsigned char c = 0; c<SRAM_CONSOLE_LINE_BUFFER_WIDTH; c++ )
			{
				if ( !(out[c] = sramReadByte()) )
				{
					break;
				}
			}
			sramStop();

			stringAt( out, 0, vertical, SRAM_CONSOLE_LINE_FONT, 0 );
			vertical += SRAM_CONSOLE_LINE_SPACING;
			if ( ++line >= SRAM_CONSOLE_LINE_BUFFER_LINES )
			{
				line = 0;
			}
		}
	}
	else if ( frameMode == FrameReplay )
	{
		struct PacketReplay *replay = (struct PacketReplay *)rnaPacket;










		replay->millisecondsRepresented = 50;

		replay->trigger[1] = 0x0F;
		replay->eye[1] = 0xF0;
		replay->solenoid1[2] = 0xF0;
		replay->solenoid2[2] = 0x0F;



		
		unsigned char interval = replay->millisecondsRepresented / 5;
		char buf[20] = "";
		unsigned int x = 22;
		unsigned int y = 0;
		for( unsigned int i=1; i<6; i++ )
		{
			dsprintf_P( buf, PSTR("%d"), interval * i );
			stringAt( buf, x, 0, 5, 0 );
			x += 22;
		}

		stringAt_P( PSTR("T"), 0, 7, 5, 0 );
		stringAt_P( PSTR("E"), 0, 13, 5, 0 );
		stringAt_P( PSTR("1"), 0, 19, 5, 0 );
		stringAt_P( PSTR("2"), 0, 25, 5, 0 );

		for( x=8; x<128; x+=3 )
		{
			setPixel( x, 8 );
		}

		for( y=8; y<32; y+=3 )
		{
			setPixel( 7, y );
		}

		char lastTrigger = (replay->trigger[0] & 0x1) ? 1 : 0;
		char lastEye = (replay->eye[0] & 0x1) ? 1 : 0;
		char lastSolenoid1 = (replay->solenoid1[0] & 0x1) ? 1 : 0;
		char lastSolenoid2 = (replay->solenoid2[0] & 0x1) ? 1 : 0;
		
		for( x=9; x<128; x++ )
		{
			int pos = ((x - 9) * replay->millisecondsRepresented) / (128 - 9);
			int v1 = pos / 8;
			int v2 = 1 << (pos % 8);
			
			if ( replay->eye[v1] & v2 )
			{
				setPixel( x, 9 );
			}
			else
			{
				setPixel( x, 13 );
			}
			char thisEye = (replay->eye[v1] & v2) ? 1 : 0;
			if ( thisEye != lastEye )
			{
				lastEye = thisEye;
				for( y=9; y<=13; y++ )
				{
					setPixel( x, y );
				}
			}
			
			if ( replay->trigger[v1] & v2 )
			{
				setPixel( x, 14 );
			}
			else
			{
				setPixel( x, 19 );
			}
			char thisTrigger = (replay->trigger[v1] & v2) ? 1 : 0;
			if ( thisTrigger != lastTrigger )
			{
				lastTrigger = thisTrigger;
				for( y=14; y<=19; y++ )
				{
					setPixel( x, y );
				}
			}

			if ( replay->solenoid1[v1] & v2 )
			{
				setPixel( x, 20 );
			}
			else
			{
				setPixel( x, 25 );
			}
			char thisSolenoid1 = (replay->solenoid1[v1] & v2) ? 1 : 0;
			if ( thisSolenoid1 != lastSolenoid1 )
			{
				lastSolenoid1 = thisSolenoid1;
				for( y=20; y<=25; y++ )
				{
					setPixel( x, y );
				}
			}

			if ( replay->solenoid2[v1] & v2 )
			{
				setPixel( x, 26 );
			}
			else
			{
				setPixel( x, 31 );
			}
			char thisSolenoid2 = (replay->solenoid2[v1] & v2) ? 1 : 0;
			if ( thisSolenoid2 != lastSolenoid2 )
			{
				lastSolenoid2 = thisSolenoid2;
				for( y=26; y<=31; y++ )
				{
					setPixel( x, y );
				}
			}
		}
	}
	else if ( frameMode == FrameMenu )
	{
		menuRender();
	}
}

//------------------------------------------------------------------------------
int __attribute__((OS_main)) main()
{
	rnaInit();
	sei();

	sramInit();
	i2cInit();
	oledInit( 0 );

	init();

	unsigned char bootStatus = eeprom_read_byte( (uint8*)EEBootStatus );
	if ( bootStatus == BootStatusInitial )
	{
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

		blit();
		
		for(;;)
		{
			if ( rnaDataReceived >= rnaDataExpected )
			{
				rnaDataReceived = 0;

				if ( rnaCommand == RNATypeEEPROMLoad )
				{
					struct PacketEEPROMLoad *load = (struct PacketEEPROMLoad *)rnaPacket;

					startWrite24c512( 0xA0, load->offset );
					for ( unsigned char i=0; i<128; i++ )
					{
						i2cWrite( load->data[i] );
					}
					i2cStop();

					clearFrameBuffer();
					char buf[32];
					dsprintf_P( buf, PSTR("offset[0x%04X]"), load->offset );
					stringAtResident( buf, 0, 0 );
					dsprintf_P( buf, PSTR("[0x%02X] -> [0x%02X]"), load->data[0], load->data[127] );
					stringAtResident( buf, 0, 8 );

					read24c512( 0xA0, load->offset, load->data, 1 );
					read24c512( 0xA0, load->offset + 127, load->data + 1, 1 );
					dsprintf_P( buf, PSTR("read[0x%02X] -> [0x%02X]"),  load->data[0], load->data[1] );
					stringAtResident( buf, 0, 16 );

					blit();
				}
			}
		}
	}


//	consolePrint_P( PSTR("0 DNA ready") );

//	stringAt( "Don't Panic", 0, 0, 7, 0 );
//	_delay_ms( 1000 );
//	blit();

	for(;;)
	{
		if ( displayDirty )
		{
			displayDirty = false;
			clearFrameBuffer();
			render();
			blit();
		}

		if ( rnaDataReceived >= rnaDataExpected )
		{
			rnaDataReceived = 0;
			char buf[32];
			
			if ( rnaCommand == RNATypeEEPROMLoad )
			{
				struct PacketEEPROMLoad *load = (struct PacketEEPROMLoad *)rnaPacket;

				startWrite24c512( 0xA0, load->offset );
				for ( unsigned int i=0; i<128; i++ )
				{
					i2cWrite( load->data[i] );
				}
				i2cStop();
			}
			else if ( rnaCommand == RNATypeConsoleString )
			{
				rnaPacket[31] = 0; // enforce termination
				dsprintf_P( buf, PSTR("%d %s"), rnaFrom, rnaPacket );
				consolePrint( buf );

				if ( frameMode == FrameConsole )
				{
					displayDirty = true;
				}
			}
			else if ( rnaCommand == RNATypeButtonStatus )
			{
				if ( frameMode == FrameMenu )
				{
					if ( menuProcessButtonState( rnaPacket[0] ) )
					{
						frameMode++;
					}
				}
				else if ( rnaPacket[0] & ButtonBitMiddle )
				{
					frameMode++;
				}

				if ( frameMode == FrameLast )
				{
					frameMode = 0;
				}

				displayDirty = true;

				dsprintf_P( buf, PSTR("%d %s %s %s"),
						   rnaFrom,
						   rnaPacket[0] & 0x1 ? "on " : "off",
						   rnaPacket[0] & 0x2 ? "on " : "off",
						   rnaPacket[0] & 0x4 ? "on " : "off" );

				if ( rnaPacket[0] == 0xFF )
				{
					dsprintf_P( buf, "" );//PSTR("3: POWER OFF!!!") );
				}

				consolePrint( buf );
				if ( frameMode == FrameConsole )
				{
					displayDirty = true;
				}
			}
			else if ( rnaCommand == RNATypeReplay )
			{
				if ( frameMode == FrameReplay )
				{
					displayDirty = true;
				}
			}
		}
	}
}
